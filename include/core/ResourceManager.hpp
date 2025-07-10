#ifndef CORE_RESOURCEMANAGER_HPP
#define CORE_RESOURCEMANAGER_HPP

#include <raylib.h>
#include <unordered_map>
#include <string>
#include <memory>
#include <functional>
#include <vector>

namespace Core {


template<typename T>
class ResourceHandle {
public:
    ResourceHandle() : m_resource(nullptr), m_id("") {}
    ResourceHandle(T* resource, const std::string& id) : m_resource(resource), m_id(id) {}
    
    T* get() const { return m_resource; }
    T* operator->() const { return m_resource; }
    T& operator*() const { return *m_resource; }
    bool isValid() const { return m_resource != nullptr; }
    const std::string& getId() const { return m_id; }

private:
    T* m_resource;
    std::string m_id;
};


class ResourceManager {
public:
    using TextureLoader = std::function<Texture2D(const std::string&)>;
    using SoundLoader = std::function<Sound(const std::string&)>;
    using MusicLoader = std::function<Music(const std::string&)>;
    using ShaderLoader = std::function<Shader(const std::string&)>;
    using FontLoader = std::function<Font(const std::string&, int)>;

    ResourceManager();
    ~ResourceManager();

    // Texture management
    ResourceHandle<Texture2D> loadTexture(const std::string& path);
    ResourceHandle<Texture2D> getTexture(const std::string& path) const;
    void unloadTexture(const std::string& path);

    // Sound management
    ResourceHandle<Sound> loadSound(const std::string& path);
    ResourceHandle<Sound> getSound(const std::string& path) const;
    void unloadSound(const std::string& path);

    // Music management
    ResourceHandle<Music> loadMusic(const std::string& path);
    ResourceHandle<Music> getMusic(const std::string& path) const;
    void unloadMusic(const std::string& path);

    // Shader management
    ResourceHandle<Shader> loadShader(const std::string& vertexPath, const std::string& fragmentPath);
    ResourceHandle<Shader> getShader(const std::string& path) const;
    void unloadShader(const std::string& path);

    // Font management
    ResourceHandle<Font> loadFont(const std::string& path, int fontSize = 32);
    ResourceHandle<Font> getFont(const std::string& path) const;
    void unloadFont(const std::string& path);

    /**
     * @brief Preload a batch of resources
     * 
     * @param texturePaths Texture file paths
     * @param soundPaths Sound file paths
     * @param musicPaths Music file paths
     */
    void preloadBatch(const std::vector<std::string>& texturePaths,
                     const std::vector<std::string>& soundPaths = {},
                     const std::vector<std::string>& musicPaths = {});

   
    void unloadAllTextures();
    void unloadAllSounds();
    void unloadAllMusic();
    void unloadAllShaders();
    void unloadAllFonts();
    void unloadAll();

    
    struct MemoryStats {
        size_t textureMemory = 0;
        size_t soundMemory = 0;
        size_t totalResources = 0;
        size_t textureCount = 0;
        size_t soundCount = 0;
        size_t musicCount = 0;
        size_t shaderCount = 0;
        size_t fontCount = 0;
    };
    
    MemoryStats getMemoryStats() const;

    
    void setHotReloadEnabled(bool enabled);

    
    void checkForHotReload();

    
    void setTextureLoader(TextureLoader loader);
    void setSoundLoader(SoundLoader loader);
    void setMusicLoader(MusicLoader loader);
    void setShaderLoader(ShaderLoader loader);
    void setFontLoader(FontLoader loader);

    
    bool hasTexture(const std::string& path) const;
    bool hasSound(const std::string& path) const;
    bool hasMusic(const std::string& path) const;
    bool hasShader(const std::string& path) const;
    bool hasFont(const std::string& path) const;

   
    void setSearchPaths(const std::vector<std::string>& paths);

  
    std::string findFile(const std::string& filename) const;

private:
    template<typename T>
    struct ResourceEntry {
        std::unique_ptr<T> resource;
        int referenceCount;
        std::string fullPath;
        size_t lastModified;
        
        ResourceEntry() : referenceCount(0), lastModified(0) {}
    };

    std::unordered_map<std::string, ResourceEntry<Texture2D>> m_textures;
    std::unordered_map<std::string, ResourceEntry<Sound>> m_sounds;
    std::unordered_map<std::string, ResourceEntry<Music>> m_music;
    std::unordered_map<std::string, ResourceEntry<Shader>> m_shaders;
    std::unordered_map<std::string, ResourceEntry<Font>> m_fonts;

    std::vector<std::string> m_searchPaths;
    bool m_hotReloadEnabled;

    TextureLoader m_textureLoader;
    SoundLoader m_soundLoader;
    MusicLoader m_musicLoader;
    ShaderLoader m_shaderLoader;
    FontLoader m_fontLoader;

    void setupDefaultLoaders();
    size_t getFileModificationTime(const std::string& path) const;
    std::string resolveResourcePath(const std::string& path) const;
    
    template<typename T>
    void incrementReference(std::unordered_map<std::string, ResourceEntry<T>>& container, const std::string& path);
    
    template<typename T>
    void decrementReference(std::unordered_map<std::string, ResourceEntry<T>>& container, const std::string& path);
};


ResourceManager& GetResourceManager();

}

#endif 
