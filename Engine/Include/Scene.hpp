#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <map>
#include <string>
#include "Vec3.hpp"

namespace ae3d
{
    namespace FileSystem
    {
        struct FileContentsData;
    }
    
    /// Contains game objects in a transform hierarchy.
    class Scene
    {
    public:
        /// Result of GetSerialized.
        enum class DeserializeResult { Success, ParseError };
        
        /// Adds a game object into the scene if it does not exist there already.
        void Add( class GameObject* gameObject );
        
        /// Ends the rendering. Called after scene.Render() and UI/line rendering etc.
        void EndFrame();

        /// Ends the rendering on Metal API. Enables line drawing after Render() and before EndRenderMetal().
        void EndRenderMetal();
        
        /// \param gameObject Game object to remove. Does nothing if it is null or doesn't exist in the scene.
        void Remove( GameObject* gameObject );
        
        /// Renders the scene.
        void Render();

        /// \param skyTexture Skybox texture.
        void SetSkybox( class TextureCube* skyTexture );
        
        /// \return Scene's contents in a textual format that can be saved into file etc.
        std::string GetSerialized() const;

        /// Deserializes a scene additively from file contents. Must be called after renderer is initialized.
        /// \param serialized Serialized scene contents.
        /// \param outGameObjects Returns game objects that were created from serialized scene contents.
        /// \param outTexture2Ds Returns texture 2Ds that were created from serialized scene contents. Caller is responsible for freeing the memory.
        /// \param outMaterials Returns materials that were created. Caller is responsible for freeing the memory.
        /// \param outMeshes Returns meshes that were created. Caller is responsible for freeing the memory.
        /// \return Result. Parsing stops on first error and successfully loaded game objects are returned.
        DeserializeResult Deserialize( const FileSystem::FileContentsData& serialized, std::vector< GameObject >& outGameObjects,
                                       std::map< std::string, class Texture2D* >& outTexture2Ds,
                                       std::map< std::string, class Material* >& outMaterials,
                                       std::vector< class Mesh* >& outMeshes ) const;
        
    private:
        void RenderWithCamera( GameObject* cameraGo, int cubeMapFace, const char* debugGroupName );
        void RenderShadowsWithCamera( GameObject* cameraGo, int cubeMapFace );
        void RenderDepthAndNormalsForAllCameras( std::vector< GameObject* >& cameras );
        void RenderDepthAndNormals( class CameraComponent* camera, const struct Matrix44& view, std::vector< unsigned > gameObjectsWithMeshRenderer,
                                    int cubeMapFace, const class Frustum& frustum );
        void GenerateAABB();

        std::vector< GameObject* > gameObjects;
        unsigned nextFreeGameObject = 0;
        TextureCube* skybox = nullptr;
        Vec3 aabbMin;
        Vec3 aabbMax;
    };
}
#endif
