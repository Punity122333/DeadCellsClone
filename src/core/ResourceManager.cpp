#include "core/ResourceManager.hpp"
#include <filesystem>
#include <iostream>
#include <ctime>

namespace Core {

ResourceManager::ResourceManager() 
    : m_hotReloadEnabled(false)
{
    setupDefaultLoaders();
    m_searchPaths.push_back("../resources/");
    m_searchPaths.push_back("./resources/");
    m_searchPaths.push_back("./");
}

ResourceManager::~ResourceManager() {
    unloadAll();
}

ResourceHandle<Texture2D> ResourceManager::loadTexture(const std::string& path) {
    std::string resolvedPath = resolveResourcePath(path);
    
    auto it = m_textures.find(resolvedPath);
    if (it != m_textures.end()) {
        incrementReference(m_textures, resolvedPath);
        return ResourceHandle<Texture2D>(it->second.resource.get(), resolvedPath);
    }

    Texture2D texture = m_textureLoader(resolvedPath);
    if (texture.id == 0) {
        std::cerr << "[ResourceManager] Failed to load texture: " << resolvedPath << std::endl;
        return ResourceHandle<Texture2D>();
    }

    auto& entry = m_textures[resolvedPath];
    entry.resource = std::make_unique<Texture2D>(texture);
    entry.fullPath = resolvedPath;
    entry.lastModified = getFileModificationTime(resolvedPath);
    incrementReference(m_textures, resolvedPath);

    return ResourceHandle<Texture2D>(entry.resource.get(), resolvedPath);
}

ResourceHandle<Texture2D> ResourceManager::getTexture(const std::string& path) const {
    std::string resolvedPath = resolveResourcePath(path);
    auto it = m_textures.find(resolvedPath);
    
    if (it != m_textures.end()) {
        return ResourceHandle<Texture2D>(it->second.resource.get(), resolvedPath);
    }
    
    return ResourceHandle<Texture2D>();
}

void ResourceManager::unloadTexture(const std::string& path) {
    std::string resolvedPath = resolveResourcePath(path);
    decrementReference(m_textures, resolvedPath);
}

ResourceHandle<Sound> ResourceManager::loadSound(const std::string& path) {
    std::string resolvedPath = resolveResourcePath(path);
    
    auto it = m_sounds.find(resolvedPath);
    if (it != m_sounds.end()) {
        incrementReference(m_sounds, resolvedPath);
        return ResourceHandle<Sound>(it->second.resource.get(), resolvedPath);
    }

    Sound sound = m_soundLoader(resolvedPath);
    if (sound.stream.buffer == nullptr) {
        std::cerr << "[ResourceManager] Failed to load sound: " << resolvedPath << std::endl;
        return ResourceHandle<Sound>();
    }

    auto& entry = m_sounds[resolvedPath];
    entry.resource = std::make_unique<Sound>(sound);
    entry.fullPath = resolvedPath;
    entry.lastModified = getFileModificationTime(resolvedPath);
    incrementReference(m_sounds, resolvedPath);

    return ResourceHandle<Sound>(entry.resource.get(), resolvedPath);
}

ResourceHandle<Sound> ResourceManager::getSound(const std::string& path) const {
    std::string resolvedPath = resolveResourcePath(path);
    auto it = m_sounds.find(resolvedPath);
    
    if (it != m_sounds.end()) {
        return ResourceHandle<Sound>(it->second.resource.get(), resolvedPath);
    }
    
    return ResourceHandle<Sound>();
}

void ResourceManager::unloadSound(const std::string& path) {
    std::string resolvedPath = resolveResourcePath(path);
    decrementReference(m_sounds, resolvedPath);
}

ResourceHandle<Music> ResourceManager::loadMusic(const std::string& path) {
    std::string resolvedPath = resolveResourcePath(path);
    
    auto it = m_music.find(resolvedPath);
    if (it != m_music.end()) {
        incrementReference(m_music, resolvedPath);
        return ResourceHandle<Music>(it->second.resource.get(), resolvedPath);
    }

    Music music = m_musicLoader(resolvedPath);
    if (music.stream.buffer == nullptr) {
        std::cerr << "[ResourceManager] Failed to load music: " << resolvedPath << std::endl;
        return ResourceHandle<Music>();
    }

    auto& entry = m_music[resolvedPath];
    entry.resource = std::make_unique<Music>(music);
    entry.fullPath = resolvedPath;
    entry.lastModified = getFileModificationTime(resolvedPath);
    incrementReference(m_music, resolvedPath);

    return ResourceHandle<Music>(entry.resource.get(), resolvedPath);
}

ResourceHandle<Music> ResourceManager::getMusic(const std::string& path) const {
    std::string resolvedPath = resolveResourcePath(path);
    auto it = m_music.find(resolvedPath);
    
    if (it != m_music.end()) {
        return ResourceHandle<Music>(it->second.resource.get(), resolvedPath);
    }
    
    return ResourceHandle<Music>();
}

void ResourceManager::unloadMusic(const std::string& path) {
    std::string resolvedPath = resolveResourcePath(path);
    decrementReference(m_music, resolvedPath);
}

ResourceHandle<Shader> ResourceManager::loadShader(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string shaderKey = vertexPath + "|" + fragmentPath;
    
    auto it = m_shaders.find(shaderKey);
    if (it != m_shaders.end()) {
        incrementReference(m_shaders, shaderKey);
        return ResourceHandle<Shader>(it->second.resource.get(), shaderKey);
    }

    Shader shader = m_shaderLoader(vertexPath + "|" + fragmentPath);
    if (shader.id == 0) {
        std::cerr << "[ResourceManager] Failed to load shader: " << vertexPath << " | " << fragmentPath << std::endl;
        return ResourceHandle<Shader>();
    }

    auto& entry = m_shaders[shaderKey];
    entry.resource = std::make_unique<Shader>(shader);
    entry.fullPath = shaderKey;
    incrementReference(m_shaders, shaderKey);

    return ResourceHandle<Shader>(entry.resource.get(), shaderKey);
}

ResourceHandle<Shader> ResourceManager::getShader(const std::string& path) const {
    auto it = m_shaders.find(path);
    
    if (it != m_shaders.end()) {
        return ResourceHandle<Shader>(it->second.resource.get(), path);
    }
    
    return ResourceHandle<Shader>();
}

void ResourceManager::unloadShader(const std::string& path) {
    decrementReference(m_shaders, path);
}

ResourceHandle<Font> ResourceManager::loadFont(const std::string& path, int fontSize) {
    std::string fontKey = path + "|" + std::to_string(fontSize);
    std::string resolvedPath = resolveResourcePath(path);
    
    auto it = m_fonts.find(fontKey);
    if (it != m_fonts.end()) {
        incrementReference(m_fonts, fontKey);
        return ResourceHandle<Font>(it->second.resource.get(), fontKey);
    }

    Font font = m_fontLoader(resolvedPath, fontSize);
    if (font.texture.id == 0) {
        std::cerr << "[ResourceManager] Failed to load font: " << resolvedPath << std::endl;
        return ResourceHandle<Font>();
    }

    auto& entry = m_fonts[fontKey];
    entry.resource = std::make_unique<Font>(font);
    entry.fullPath = resolvedPath;
    entry.lastModified = getFileModificationTime(resolvedPath);
    incrementReference(m_fonts, fontKey);

    return ResourceHandle<Font>(entry.resource.get(), fontKey);
}

ResourceHandle<Font> ResourceManager::getFont(const std::string& path) const {
    auto it = m_fonts.find(path);
    
    if (it != m_fonts.end()) {
        return ResourceHandle<Font>(it->second.resource.get(), path);
    }
    
    return ResourceHandle<Font>();
}

void ResourceManager::unloadFont(const std::string& path) {
    decrementReference(m_fonts, path);
}

void ResourceManager::preloadBatch(const std::vector<std::string>& texturePaths,
                                  const std::vector<std::string>& soundPaths,
                                  const std::vector<std::string>& musicPaths) {
    for (const auto& path : texturePaths) {
        loadTexture(path);
    }
    
    for (const auto& path : soundPaths) {
        loadSound(path);
    }
    
    for (const auto& path : musicPaths) {
        loadMusic(path);
    }
}

void ResourceManager::unloadAllTextures() {
    for (auto& [path, entry] : m_textures) {
        if (entry.resource) {
            UnloadTexture(*entry.resource);
        }
    }
    m_textures.clear();
}

void ResourceManager::unloadAllSounds() {
    for (auto& [path, entry] : m_sounds) {
        if (entry.resource) {
            UnloadSound(*entry.resource);
        }
    }
    m_sounds.clear();
}

void ResourceManager::unloadAllMusic() {
    for (auto& [path, entry] : m_music) {
        if (entry.resource) {
            UnloadMusicStream(*entry.resource);
        }
    }
    m_music.clear();
}

void ResourceManager::unloadAllShaders() {
    for (auto& [path, entry] : m_shaders) {
        if (entry.resource) {
            UnloadShader(*entry.resource);
        }
    }
    m_shaders.clear();
}

void ResourceManager::unloadAllFonts() {
    for (auto& [path, entry] : m_fonts) {
        if (entry.resource) {
            UnloadFont(*entry.resource);
        }
    }
    m_fonts.clear();
}

void ResourceManager::unloadAll() {
    unloadAllTextures();
    unloadAllSounds();
    unloadAllMusic();
    unloadAllShaders();
    unloadAllFonts();
}

ResourceManager::MemoryStats ResourceManager::getMemoryStats() const {
    MemoryStats stats;
    
    stats.textureCount = m_textures.size();
    stats.soundCount = m_sounds.size();
    stats.musicCount = m_music.size();
    stats.shaderCount = m_shaders.size();
    stats.fontCount = m_fonts.size();
    stats.totalResources = stats.textureCount + stats.soundCount + stats.musicCount + stats.shaderCount + stats.fontCount;
    
    for (const auto& [path, entry] : m_textures) {
        if (entry.resource) {
            const auto& tex = *entry.resource;
            stats.textureMemory += tex.width * tex.height * 4; 
        }
    }
    
    return stats;
}

void ResourceManager::setHotReloadEnabled(bool enabled) {
    m_hotReloadEnabled = enabled;
}

void ResourceManager::checkForHotReload() {
    if (!m_hotReloadEnabled) return;

    for (auto& [path, entry] : m_textures) {
        size_t currentModTime = getFileModificationTime(entry.fullPath);
        if (currentModTime > entry.lastModified) {

            Texture2D newTexture = m_textureLoader(entry.fullPath);
            if (newTexture.id != 0) {
                UnloadTexture(*entry.resource);
                *entry.resource = newTexture;
                entry.lastModified = currentModTime;
                std::cout << "[ResourceManager] Hot-reloaded texture: " << path << std::endl;
            }
        }
    }
    

}

void ResourceManager::setTextureLoader(TextureLoader loader) {
    m_textureLoader = std::move(loader);
}

void ResourceManager::setSoundLoader(SoundLoader loader) {
    m_soundLoader = std::move(loader);
}

void ResourceManager::setMusicLoader(MusicLoader loader) {
    m_musicLoader = std::move(loader);
}

void ResourceManager::setShaderLoader(ShaderLoader loader) {
    m_shaderLoader = std::move(loader);
}

void ResourceManager::setFontLoader(FontLoader loader) {
    m_fontLoader = std::move(loader);
}

bool ResourceManager::hasTexture(const std::string& path) const {
    std::string resolvedPath = resolveResourcePath(path);
    return m_textures.find(resolvedPath) != m_textures.end();
}

bool ResourceManager::hasSound(const std::string& path) const {
    std::string resolvedPath = resolveResourcePath(path);
    return m_sounds.find(resolvedPath) != m_sounds.end();
}

bool ResourceManager::hasMusic(const std::string& path) const {
    std::string resolvedPath = resolveResourcePath(path);
    return m_music.find(resolvedPath) != m_music.end();
}

bool ResourceManager::hasShader(const std::string& path) const {
    return m_shaders.find(path) != m_shaders.end();
}

bool ResourceManager::hasFont(const std::string& path) const {
    return m_fonts.find(path) != m_fonts.end();
}

void ResourceManager::setSearchPaths(const std::vector<std::string>& paths) {
    m_searchPaths = paths;
}

std::string ResourceManager::findFile(const std::string& filename) const {
    std::cout << "[ResourceManager] Looking for file: " << filename << std::endl;
    
    // First try the filename as-is
    if (std::filesystem::exists(filename)) {
        std::cout << "[ResourceManager] Found file directly: " << filename << std::endl;
        return filename;
    }
    
    // Then try each search path
    for (const auto& searchPath : m_searchPaths) {
        std::string fullPath = searchPath + filename;
        std::cout << "[ResourceManager] Trying path: " << fullPath << std::endl;
        if (std::filesystem::exists(fullPath)) {
            std::cout << "[ResourceManager] Found file: " << fullPath << std::endl;
            return fullPath;
        }
    }
    
    std::cout << "[ResourceManager] File not found: " << filename << std::endl;
    return ""; // Not found
}

void ResourceManager::setupDefaultLoaders() {
    m_textureLoader = [](const std::string& path) -> Texture2D {
        return LoadTexture(path.c_str());
    };
    
    m_soundLoader = [](const std::string& path) -> Sound {
        return LoadSound(path.c_str());
    };
    
    m_musicLoader = [](const std::string& path) -> Music {
        return LoadMusicStream(path.c_str());
    };
    
    m_shaderLoader = [](const std::string& path) -> Shader {
        size_t separator = path.find('|');
        if (separator != std::string::npos) {
            std::string vertexPath = path.substr(0, separator);
            std::string fragmentPath = path.substr(separator + 1);
            
            const char* vs = vertexPath.empty() ? nullptr : vertexPath.c_str();
            const char* fs = fragmentPath.empty() ? nullptr : fragmentPath.c_str();
            
            return LoadShader(vs, fs);
        }
        return LoadShader(nullptr, path.c_str());
    };
    
    m_fontLoader = [](const std::string& path, int fontSize) -> Font {
        return LoadFontEx(path.c_str(), fontSize, nullptr, 0);
    };
}

size_t ResourceManager::getFileModificationTime(const std::string& path) const {
    try {
        auto fileTime = std::filesystem::last_write_time(path);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            fileTime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        return std::chrono::system_clock::to_time_t(sctp);
    } catch (const std::filesystem::filesystem_error&) {
        return 0;
    }
}

std::string ResourceManager::resolveResourcePath(const std::string& path) const {
    std::string found = findFile(path);
    return found.empty() ? path : found;
}

template<typename T>
void ResourceManager::incrementReference(std::unordered_map<std::string, ResourceEntry<T>>& container, const std::string& path) {
    auto it = container.find(path);
    if (it != container.end()) {
        it->second.referenceCount++;
    }
}

template<typename T>
void ResourceManager::decrementReference(std::unordered_map<std::string, ResourceEntry<T>>& container, const std::string& path) {
    auto it = container.find(path);
    if (it != container.end()) {
        it->second.referenceCount--;
        if (it->second.referenceCount <= 0) {
            // Unload the resource
            if (it->second.resource) {
                if constexpr (std::is_same_v<T, Texture2D>) {
                    UnloadTexture(*it->second.resource);
                } else if constexpr (std::is_same_v<T, Sound>) {
                    UnloadSound(*it->second.resource);
                } else if constexpr (std::is_same_v<T, Music>) {
                    UnloadMusicStream(*it->second.resource);
                } else if constexpr (std::is_same_v<T, Shader>) {
                    UnloadShader(*it->second.resource);
                } else if constexpr (std::is_same_v<T, Font>) {
                    UnloadFont(*it->second.resource);
                }
            }
            container.erase(it);
        }
    }
}

} // namespace Core
