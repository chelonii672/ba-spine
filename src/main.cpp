#include <SFML/Graphics.hpp>
#include "spine/spine.h"
#include "spine/spine-sfml.h"

#include <iostream>
#include <memory>
#include <algorithm>
#include <vector>
#include <iomanip>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;


// ========================================================
// Helper functions

template<typename T, typename... Args>
std::unique_ptr<T> makeUnique(Args&&... args) {
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

std::shared_ptr<spine::SkeletonData>
readSkeletonBinaryData(fs::path filepath, spine::Atlas* atlas, float scale) {
    spine::SkeletonBinary binary(atlas);
    binary.setScale(scale);

    auto skeleton_data = binary.readSkeletonDataFile(filepath.string().c_str());
    if (!skeleton_data) {
        std::cerr << "error: " << binary.getError().buffer() << '\n';
        return nullptr;
    }

    return std::shared_ptr<spine::SkeletonData>(skeleton_data);
}

std::string
toLowercase(const std::string& str) {
    auto result = str;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });

    return result;
}

bool
contains(const std::string& str, const std::vector<std::string>& values) {
    auto it = std::find_if(values.begin(), values.end(), [&](const std::string& s) { return str.find(s) != std::string::npos; });
    return it != values.end();
}

/// Get file's basename
/// e.g. "character_spr.skel" -> "character_spr"
std::string
getBasename(const fs::path& filepath) {
    auto lower_basename = toLowercase(filepath.stem().string());

    // remove suffix "_spr"
    {
        size_t char_suffix_pos = lower_basename.find("_spr");
        if (char_suffix_pos != std::string::npos)
            lower_basename = lower_basename.substr(0, char_suffix_pos);
    }

    return lower_basename;
}

/// Generate filename
/// e.g. "character^animation.extension" or if animated "character^animation-0001.extension"
std::string
getFilename(
    const std::string& basename,
    const std::string& animation_name,
    const std::string& file_extension,

    int16_t index = -1,
    uint8_t prefix_width = 6
) {
    std::stringstream result;

    std::ios original_fmt(nullptr);
    original_fmt.copyfmt(result);

    result << basename << "--" << animation_name;
    if (index != -1) {
        result << '-' << std::setfill('0') << std::setw(prefix_width) << index;
        result.copyfmt(original_fmt);
    }
    result << '.' << file_extension;

    return result.str();
}

// Check if animation is static
// Might be have some hardcode
bool
isStaticAnimation(spine::SkeletonData* skeleton_data, const std::string& animation_name) {
    std::string lowercase_animation_name = toLowercase(animation_name);

    if (contains(lowercase_animation_name, { "eyeclose", "eye_close" }))
        return false;

    if (contains(lowercase_animation_name, { "idle" }))
        return skeleton_data->getAnimations().size() <= 1;

    return true;
}


// ========================================================
// `draw` functions

/// Draw for character's sprite using in stories
/// Create subdirectory in `dest_dir` to contain its images
///
/// For animated animations, create 1 more deep level in its directory
/// and contain its frames
void
drawCharacterSprite(
    fs::path dest_dir,
    const std::string& file_extension,

    fs::path skel_filepath,
    fs::path atlas_filepath,
    float scale = 0.5f
) {
    // load atlas file
    spine::SFMLTextureLoader texture_loader;
    auto atlas = makeUnique<spine::Atlas>(atlas_filepath.string().c_str(), &texture_loader);

    SP_UNUSED(atlas.get());

    // load skeleton data
    auto skeleton_data = readSkeletonBinaryData(skel_filepath, atlas.get(), scale);
    if (!skeleton_data) {
        std::cerr << "failed to load skel '" << skel_filepath.filename() << "'\n";
        return;
    }

    // load skeleton drawable
    spine::SkeletonDrawable drawable(skeleton_data.get());
    drawable.setUsePremultipliedAlpha(false);

    // load skeleton
    auto skeleton = drawable.skeleton;

    // get image x, y, width, height
    skeleton->setPosition(0, 0);
    skeleton->updateWorldTransform();

    float x, y, width, height;
    {
        spine::Vector<float> temp_val;
        skeleton->getBounds(x, y, width, height, temp_val);
    }

    float distance_to_edge = 20.0f;
    skeleton->setPosition(distance_to_edge/2 - x, distance_to_edge/2 - y);
    skeleton->updateWorldTransform();

    // create renderer
    sf::RenderTexture renderer;
    if (!renderer.create(width + distance_to_edge, height + distance_to_edge)) {
        std::cerr << "failed to create render(width: " << width + distance_to_edge
                  << ", height: " << height + distance_to_edge << ")\n";
        return;
    }

    // create character's directory
    auto basename = getBasename(skel_filepath);
    auto char_dest_dir = dest_dir / basename;
    if (!fs::exists(char_dest_dir))
        fs::create_directory(char_dest_dir);
    
    // notify to user
    std::cout << "\n----- start drawing '" << basename << "' -----\n";

    // draw animations to file
    for (size_t i = 0; i < skeleton_data->getAnimations().size(); i++) {
        auto animation = skeleton_data->getAnimations()[i];
        auto track_entry = drawable.state->setAnimation(0, animation, false);

        std::string animation_name = animation->getName().buffer();

        // draw for static animation
        if (isStaticAnimation(skeleton_data.get(), animation_name)) {
            // notify to user
            drawable.state->setAnimation(0, animation, false);
            std::cout << "start drawing static animation '" << animation_name << "'\n";

            // start drawing
            drawable.update(0);

            renderer.clear(sf::Color::Transparent);
            renderer.draw(drawable, sf::BlendAlpha);
            renderer.display();

            // get filepath
            auto file_destpath = char_dest_dir / getFilename(basename, animation_name, file_extension);
            renderer.getTexture().copyToImage().saveToFile(file_destpath.string());

            // notify to user
            std::cout << "done drawing static animation '" << animation_name << "'\n\n";
        }

        // TODO: Learn using libav to dump animated L2D
        // draw for animated animation
        else {
            std::cout << "drawing animated animation hasn't implemented yet\n\n";

            // // create subdirectory contains animation's frames
            // auto anim_dir_path = char_dest_dir / getFilename(basename, animation_name, file_extension);
            // if (!fs::exists(anim_dir_path))
            //     fs::create_directory(anim_dir_path);

            // // change animation timescale
            // track_entry->setTimeScale(0.2f);

            // // calculate duration and delta time
            // float animation_duration = track_entry->getAnimation()->getDuration();
            // // 60 fps
            // // e.g. if animation has duration is 3 second
            // // -> 3*60 = 180 frames in total
            // // -> increase 3/180 = 0.01(6)s each frame
            // double delta_inc_duration = (double)(animation_duration / (animation_duration * 60));

            // // notify to user
            // std::cout << "start drawing animated animation (" << animation_duration << "s) '"
            //           << animation_name << "'\n";

            // // start drawing frames
            // int16_t index = 0;
            // double delta_time = index * delta_inc_duration;
            // while (delta_time <= animation_duration) {
            //     drawable.update(delta_time);

            //     renderer.clear(sf::Color::Transparent);
            //     renderer.draw(drawable, sf::BlendAlpha);
            //     renderer.display();

            //     // get filepath
            //     auto file_destpath = anim_dir_path / getFilename(basename, animation_name, file_extension, index);
            //     renderer.getTexture().copyToImage().saveToFile(file_destpath.string());

            //     // notify to user
            //     std::cout << "done drawing frame (" << delta_time << "s)\n";

            //     // increase index
            //     index++;
            //     delta_time = index * delta_inc_duration;
            // }
        
            // // notify to user
            // std::cout << "done drawing animated animation '" << animation_name << "'\n\n";
        }
    }
}

// TODO: Learn using libav to dump animated L2D
// /// Draw for student's memorial lobby
// /// Create subdirectory in `dest_dir` to contain animation's frames
// void
// drawL2D(
//     fs::path dest_dir,
//     const std::string& file_extension,

//     fs::path skel_filepath,
//     fs::path atlas_filepath,
//     float scale = 0.5f
// ) {
//     // load atlas file
//     spine::SFMLTextureLoader texture_loader;
//     auto atlas = makeUnique<spine::Atlas>(atlas_filepath.string().c_str(), &texture_loader);

//     SP_UNUSED(atlas.get());

//     // load skeleton data
//     auto skeleton_data = readSkeletonBinaryData(skel_filepath, atlas.get(), scale);
//     if (!skeleton_data) {
//         std::cerr << "failed to load skel '" << skel_filepath.filename() << "'\n";
//         return;
//     }

//     // load skeleton drawable
//     spine::SkeletonDrawable drawable(skeleton_data.get());
//     drawable.timeScale = 1.0f;
//     drawable.setUsePremultipliedAlpha(false);

//     // load skeleton
//     auto skeleton = drawable.skeleton;

//     // get image x, y, width, height
//     skeleton->setPosition(0, 0);
//     skeleton->updateWorldTransform();

//     float x, y, width, height;
//     {
//         spine::Vector<float> temp_val;
//         skeleton->getBounds(x, y, width, height, temp_val);
//     }

//     float distance_to_edge = 20.0f;
//     skeleton->setPosition(distance_to_edge/2 - x, distance_to_edge/2 - y);
//     skeleton->updateWorldTransform();

//     // create renderer
//     sf::RenderTexture renderer;
//     if (!renderer.create(width + distance_to_edge, height + distance_to_edge)) {
//         std::cerr << "failed to create render(width: " << width + distance_to_edge
//                   << ", height: " << height + distance_to_edge << ")\n";
//         return;
//     }

//     // create character's directory
//     auto basename = getBasename(skel_filepath);
//     auto char_dest_dir = dest_dir / basename;
//     if (!fs::exists(char_dest_dir))
//         fs::create_directory(char_dest_dir);
    
//     // notify to user
//     std::cout << "\n----- start drawing '" << basename << "' -----\n";

//     // draw animations to file
//     for (size_t i = 0; i < skeleton_data->getAnimations().size(); i++) {
//         auto animation = skeleton_data->getAnimations()[i];
//         drawable.state->setAnimation(0, animation, false);

//         std::string animation_name = animation->getName().buffer();
//         float animation_duration = animation->getDuration();

//         // create subdirectory contains animation's frames
//         auto anim_dir_path = char_dest_dir / getFilename(basename, animation_name, file_extension);
//         if (!fs::exists(anim_dir_path))
//             fs::create_directory(anim_dir_path);

//         // 60 fps
//         // e.g. if animation has duration is 3 second
//         // -> 3*60 = 180 frames in total
//         // -> increase 3/180 = 0.01(6)s each frame
//         double delta_inc_duration = (double)(animation_duration / (animation_duration * 60));

//         // notify to user
//         std::cout << "start drawing animation (" << animation_duration << "s) '" << animation_name << "'\n";

//         // start drawing frames
//         int16_t index = 0;
//         double delta_time = index * delta_inc_duration;
//         while (delta_time <= animation_duration) {
//             drawable.update(delta_time);

//             renderer.clear(sf::Color::Transparent);
//             renderer.draw(drawable, sf::BlendAlpha);
//             renderer.display();

//             // get filepath
//             auto file_destpath = anim_dir_path / getFilename(basename, animation_name, file_extension, index);
//             renderer.getTexture().copyToImage().saveToFile(file_destpath.string());

//             // notify to user
//             std::cout << "done drawing frame (" << delta_time << "s)\n";

//             // increase index
//             index++;
//             delta_time = index * delta_inc_duration;
//         }
    
//         // notify to user
//         std::cout << "done drawing animation '" << animation_name << "'\n\n";
//     }
// }

// ========================================================

fs::path
getAtlasPath(const fs::path& skel_filepath) {
    auto skel_parent_dir = skel_filepath.parent_path();
    auto skel_basename = skel_filepath.stem().string();

    return skel_parent_dir / (skel_basename + ".atlas");
}

int
main(int argc, char* argv[]) {
    // default value
    fs::path input_dir = ".";
    fs::path dest_dir = "result";
    std::string file_extension = "png";
    float scale = 0.5f;

    // cmd args
    if (argc >= 2)
        input_dir = argv[1];
    if (argc >= 3)
        dest_dir = argv[2];
    if (argc >= 4)
        file_extension = argv[3];
    if (argc >= 5)
        scale = std::stof(argv[4]);

    // create result dir
    if (!fs::exists(dest_dir))
        fs::create_directory(dest_dir);

    // walk through input_dir
    for (auto& entry : fs::recursive_directory_iterator(input_dir)) if (entry.path().extension().string() == ".skel") {
        auto skel_filepath = entry.path();

        // skip L2D (aka files end with "home" or "scene")
        auto lower_skel_basename = toLowercase(skel_filepath.stem().string());
        if (contains(lower_skel_basename, { "home", "scene" })) continue;

        // get atlas filepath
        auto atlas_filepath = getAtlasPath(skel_filepath);

        // start drawing
        drawCharacterSprite(dest_dir, file_extension, skel_filepath, atlas_filepath, scale);
    }
}