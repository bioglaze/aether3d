#include <iostream>
#include <fstream>
#include <cassert>
#include "fbxsdk.h"
#include "../common.hpp"

using namespace ae3d;

FbxManager* sdkManager = nullptr;
FbxScene* scene = nullptr;

// http://oddeffects.blogspot.fi/2013/10/fbx-sdk-tips.html
Matrix44 GetTransform( FbxNode* node )
{
    FbxMatrix matTransform = node->GetAnimationEvaluator()->GetNodeLocalTransform( node );
    
    FbxVector4 t = node->GetGeometricTranslation( FbxNode::eSourcePivot );
    FbxVector4 r = node->GetGeometricRotation( FbxNode::eSourcePivot );
    FbxVector4 s = node->GetGeometricScaling( FbxNode::eSourcePivot );
    FbxMatrix matGeometricOffset( t, r, s );
    
    FbxMatrix nodeMatrix = /*parentMatrix */ matTransform;
    FbxMatrix worldMatrix = nodeMatrix * matGeometricOffset;
    
    double* mat = (double*)worldMatrix;
    
    Matrix44 transform;
    
    for (int i = 0; i < 16; ++i)
    {
        transform.m[ i ] = static_cast< float >( mat[ i ] );
    }

    return transform;
}

FbxScene* ImportScene( const std::string& path )
{
    sdkManager = FbxManager::Create();

    if (sdkManager)
    {
        std::cout << "Autodesk FBX SDK version " << sdkManager->GetVersion() << std::endl;
    }

    FbxIOSettings* ios = FbxIOSettings::Create( sdkManager, IOSROOT );
    sdkManager->SetIOSettings( ios );

    scene = FbxScene::Create( sdkManager, "My Scene" );

    if (!scene)
    {
        std::cerr << "Unable to create SDK scene to hold imported meshes." << std::endl;
        exit( 1 );
    }

    int sdkMajor, sdkMinor, sdkRevision;
    FbxManager::GetFileFormatVersion( sdkMajor, sdkMinor, sdkRevision );

    // Create an importer.
    FbxImporter* importer = FbxImporter::Create( sdkManager, "" );

    // Initialize the importer by providing a filename.
    const bool importStatus = importer->Initialize( path.c_str(), -1, sdkManager->GetIOSettings() );
    int fileMajor, fileMinor, fileRevision;
    importer->GetFileVersion( fileMajor, fileMinor, fileRevision );

    if (!importStatus)
    {
        FbxString error = importer->GetStatus().GetErrorString();
        std::cerr << "Could not import " << path << ": " << error.Buffer() << std::endl;

        if (importer->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
        {
            std::cerr << "Invalid version." << std::endl;
            std::cerr << "File version: " << fileMajor << "." << fileMinor << "." << fileRevision << std::endl;
            std::cerr << "SDK version: " << sdkMajor << "." << sdkMinor << "." << sdkRevision << std::endl;
            exit( 1 );
        }
    }

    if (!importer->IsFBX())
    {
        std::cerr << "Unrecognized FBX format" << std::endl;
        exit( 1 );
    }

    const bool status = importer->Import( scene );

    if (!status)
    {
        std::cerr << "Import failed." << std::endl;
        exit( 1 );
    }
    
    FbxGeometryConverter clsConverter( sdkManager );
    clsConverter.Triangulate( scene, true );

    importer->Destroy();
    return scene;
}

void ProcessVertices( FbxNode* node )
{
    assert( node );
    assert( !gMeshes.empty() );
    
    FbxMesh* mesh = node->GetMesh();
    const int vertexCount = mesh->GetControlPointsCount();

    gMeshes.back().vertex.resize( vertexCount );

    for (int i = 0; i < vertexCount; ++i)
    {
        Vec3 vertex;
        auto point = mesh->GetControlPointAt( i );
        vertex.x = static_cast<float>( point.mData[ 0 ] );
        vertex.y = static_cast<float>( point.mData[ 1 ] );
        vertex.z = static_cast<float>( point.mData[ 2 ] );
        gMeshes.back().vertex[ i ] = vertex;
    }
    
    Matrix44 transform = GetTransform( node );
    
    for (int i = 0; i < vertexCount; ++i)
    {
        Matrix44::TransformPoint( gMeshes.back().vertex[ i ], transform, &gMeshes.back().vertex[ i ] );
    }
}

void ProcessNormal( FbxMesh* mesh, int vertexIndex, int vertexCounter, Vec3& outNormal )
{
    FbxGeometryElementNormal* vertexNormal = mesh->GetElementNormal( 0 );

    switch (vertexNormal->GetMappingMode())
    {
    case FbxGeometryElement::eByControlPoint:
        switch (vertexNormal->GetReferenceMode())
        {
        case FbxGeometryElement::eDirect:
        {
            const auto vn = vertexNormal->GetDirectArray().GetAt( vertexIndex );
            outNormal.x = static_cast<float>( vn.mData[ 0 ] );
            outNormal.y = static_cast<float>( vn.mData[ 1 ] );
            outNormal.z = static_cast<float>( vn.mData[ 2 ]);
        }
            break;

        case FbxGeometryElement::eIndexToDirect:
        {
            const int index = vertexNormal->GetIndexArray().GetAt( vertexIndex );
            const auto vn = vertexNormal->GetDirectArray().GetAt( index );
            outNormal.x = static_cast<float>( vn.mData[ 0 ] );
            outNormal.y = static_cast<float>( vn.mData[ 1 ] );
            outNormal.z = static_cast<float>( vn.mData[ 2 ] );
        }
            break;

        default:
            assert( !"invalid reference." );
            exit( 1 );
        }
        break;

    case FbxGeometryElement::eByPolygonVertex:
        switch (vertexNormal->GetReferenceMode())
        {
        case FbxGeometryElement::eDirect:
        {
            outNormal.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt( vertexCounter ).mData[ 0 ]);
            outNormal.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt( vertexCounter ).mData[ 1 ]);
            outNormal.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt( vertexCounter ).mData[ 2 ]);
        }
            break;

        case FbxGeometryElement::eIndexToDirect:
        {
            int index = vertexNormal->GetIndexArray().GetAt( vertexCounter );
            outNormal.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt( index ).mData[ 0 ]);
            outNormal.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt( index ).mData[ 1 ]);
            outNormal.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt( index ).mData[ 2 ]);
        }
            break;

        default:
            assert( !"Invalid reference." );
            exit( 1 );
        }
        break;
            
        case FbxGeometryElement::eNone:
        case FbxGeometryElement::eByPolygon:
        case FbxGeometryElement::eByEdge:
        case FbxGeometryElement::eAllSame:
            std::cerr << "Unhandled mapping mode for vertex normal.";
            exit( 1 );
            break;
    }
}

void ProcessUV( FbxMesh* mesh, int vertexIndex, int uvChannel, int uvLayer, TexCoord& outUV )
{
    if (uvLayer > 1 || mesh->GetElementUVCount() <= uvLayer)
    {
        assert( !"Invalid reference." );
        exit( 1 );
    }
    FbxGeometryElementUV* vertexUV = mesh->GetElementUV( uvLayer );

    switch (vertexUV->GetMappingMode())
    {
    case FbxGeometryElement::eByControlPoint:
        switch (vertexUV->GetReferenceMode())
        {
        case FbxGeometryElement::eDirect:
        {
            outUV.u = static_cast<float>(vertexUV->GetDirectArray().GetAt( vertexIndex ).mData[ 0 ]);
            outUV.v = 1 - static_cast<float>(vertexUV->GetDirectArray().GetAt( vertexIndex ).mData[ 1 ]);
        }
            break;

        case FbxGeometryElement::eIndexToDirect:
        {
            int index = vertexUV->GetIndexArray().GetAt( vertexIndex );
            outUV.u = static_cast<float>(vertexUV->GetDirectArray().GetAt( index ).mData[ 0 ]);
            outUV.v = 1 - static_cast<float>(vertexUV->GetDirectArray().GetAt( index ).mData[ 1 ]);
        }
            break;

        default:
            assert( !"Invalid reference." );
            exit( 1 );
        }
        break;

    case FbxGeometryElement::eByPolygonVertex:
        switch (vertexUV->GetReferenceMode())
        {
        case FbxGeometryElement::eDirect:
        case FbxGeometryElement::eIndexToDirect:
        {
            outUV.u = static_cast<float>(vertexUV->GetDirectArray().GetAt( uvChannel ).mData[ 0 ]);
            outUV.v = 1 - static_cast<float>(vertexUV->GetDirectArray().GetAt( uvChannel ).mData[ 1 ]);
        }
            break;

        default:
            assert( !"Invalid reference." );
            exit( 1 );
        }
        break;

        case FbxGeometryElement::eNone:
        case FbxGeometryElement::eByPolygon:
        case FbxGeometryElement::eByEdge:
        case FbxGeometryElement::eAllSame:
            std::cerr << "Unhandled mapping mode for texture coordinate.";
            exit( 1 );
            break;
    }
}

void ImportMeshNodes( FbxNode* node )
{
    if (!node)
    {
        return;
    }
    
    FbxMesh* mesh = node->GetMesh();

    if (mesh)
    {
        if (mesh->GetElementNormalCount() == 0)
        {
            std::cerr << "Mesh " << mesh->GetName() << " was exported without normals, aborting." << std::endl;
            exit( 1 );
        }

        gMeshes.push_back( Mesh() );
        gMeshes.back().name = mesh->GetName();

        ProcessVertices( node );

        const int triangleCount = mesh->GetPolygonCount();
        int vertexCounter = 0;
        
        for (int i = 0; i < triangleCount; ++i)
        {
            Vec3 normal[ 3 ];
            TexCoord uv[ 3 ];
            Face face;

            for (int j = 0; j < 3; ++j)
            {
                const int vertexIndex = mesh->GetPolygonVertex( i, j );
                const int uvChannel = 0;

                ProcessNormal( mesh, vertexIndex, vertexCounter, normal[ j ] );
                ProcessUV( mesh, vertexIndex, mesh->GetTextureUVIndex( i, j ), uvChannel, uv[ j ] );

                gMeshes.back().vnormal.push_back( normal[ j ] );
                gMeshes.back().tcoord.push_back( uv[ j ] );

                face.vInd[ j ] = (unsigned short)vertexIndex;
                face.vnInd[ j ] = (unsigned short)vertexCounter;
                face.uvInd[ j ] = (unsigned short)vertexCounter;
                ++vertexCounter;
            }
            
            gMeshes.back().face.push_back( face );
        }
        
        Matrix44 transform = GetTransform( node );
        
        for (std::size_t i = 0; i < gMeshes.back().vnormal.size(); ++i)
        {
            Matrix44::TransformDirection( gMeshes.back().vnormal[ i ], transform, &gMeshes.back().vnormal[ i ] );
        }
    }

    for (int i = 0; i < node->GetChildCount(); ++i)
    {
        ImportMeshNodes( node->GetChild( i ) );
    }
}

struct Joint
{
    FbxNode* node = nullptr;
    FbxAMatrix globalBindposeInverse;
    int parentIndex = 0;
    std::string name;
};

struct Skeleton
{
    std::vector< Joint > joints;
};

struct BlendingIndexWeightPair
{
    int blendingIndex = 0;
    float blendingWeight = 0;
};

struct CtrlPoint
{
    Vec3 mPosition;
    std::vector<BlendingIndexWeightPair> mBlendingInfo;
};

Skeleton skeleton;
std::vector< CtrlPoint* > mControlPoints;

FbxAMatrix GetGeometryTransformation( FbxNode* inNode )
{
    assert( inNode && "GetGeometryTransformation: node is null" );

    const FbxVector4 lT = inNode->GetGeometricTranslation( FbxNode::eSourcePivot );
    const FbxVector4 lR = inNode->GetGeometricRotation( FbxNode::eSourcePivot );
    const FbxVector4 lS = inNode->GetGeometricScaling( FbxNode::eSourcePivot );

    return FbxAMatrix( lT, lR, lS );
}

void ProcessControlPoints( FbxNode* inNode )
{
    FbxMesh* currMesh = inNode->GetMesh();
    unsigned int ctrlPointCount = currMesh->GetControlPointsCount();
    for (unsigned int i = 0; i < ctrlPointCount; ++i)
    {
        CtrlPoint* currCtrlPoint = new CtrlPoint();
        Vec3 currPosition;
        currPosition.x = static_cast<float>(currMesh->GetControlPointAt( i ).mData[ 0 ]);
        currPosition.y = static_cast<float>(currMesh->GetControlPointAt( i ).mData[ 1 ]);
        currPosition.z = static_cast<float>(currMesh->GetControlPointAt( i ).mData[ 2 ]);
        currCtrlPoint->mPosition = currPosition;
        mControlPoints[ i ] = currCtrlPoint;
    }
}

void ProcessJointsAndAnimations( FbxNode* inNode )
{
    FbxMesh* currMesh = inNode->GetMesh();
    unsigned int numOfDeformers = currMesh->GetDeformerCount();
    // This geometry transform is something I cannot understand
    // I think it is from MotionBuilder
    // If you are using Maya for your models, 99% this is just an
    // identity matrix
    // But I am taking it into account anyways......
    FbxAMatrix geometryTransform = GetGeometryTransformation( inNode );

    // A deformer is a FBX thing, which contains some clusters
    // A cluster contains a link, which is basically a joint
    // Normally, there is only one deformer in a mesh
    for (unsigned int deformerIndex = 0; deformerIndex < numOfDeformers; ++deformerIndex)
    {
        // There are many types of deformers in Maya,
        // We are using only skins, so we see if this is a skin
        FbxSkin* currSkin = reinterpret_cast<FbxSkin*>(currMesh->GetDeformer( deformerIndex, FbxDeformer::eSkin ));
        if (!currSkin)
        {
            continue;
        }

        unsigned int numOfClusters = currSkin->GetClusterCount();
        for (unsigned int clusterIndex = 0; clusterIndex < numOfClusters; ++clusterIndex)
        {
            FbxCluster* currCluster = currSkin->GetCluster( clusterIndex );
            std::string currJointName = currCluster->GetLink()->GetName();
            unsigned int currJointIndex = 0;// FindJointIndexUsingName( currJointName );
            FbxAMatrix transformMatrix;
            FbxAMatrix transformLinkMatrix;
            FbxAMatrix globalBindposeInverseMatrix;

            currCluster->GetTransformMatrix( transformMatrix );	// The transformation of the mesh at binding time
            currCluster->GetTransformLinkMatrix( transformLinkMatrix );	// The transformation of the cluster(joint) at binding time from joint space to world space
            globalBindposeInverseMatrix = transformLinkMatrix.Inverse() * transformMatrix * geometryTransform;

            // Update the information in mSkeleton 
            skeleton.joints[ currJointIndex ].globalBindposeInverse = globalBindposeInverseMatrix;
            skeleton.joints[ currJointIndex ].node = currCluster->GetLink();

            // Associate each joint with the control points it affects
            unsigned int numOfIndices = currCluster->GetControlPointIndicesCount();
            for (unsigned int i = 0; i < numOfIndices; ++i)
            {
                BlendingIndexWeightPair currBlendingIndexWeightPair;
                currBlendingIndexWeightPair.blendingIndex = currJointIndex;
                currBlendingIndexWeightPair.blendingWeight = static_cast< float >( currCluster->GetControlPointWeights()[ i ] );
                mControlPoints[ currCluster->GetControlPointIndices()[ i ] ]->mBlendingInfo.push_back( currBlendingIndexWeightPair );
            }

            // Get animation information
            // Now only supports one take
            FbxAnimStack* currAnimStack = 0;//B = scene->GetSrcObject<FbxAnimStack>( 0 );
            FbxString animStackName = currAnimStack->GetName();
            std::string mAnimationName = animStackName.Buffer();
            std::cout << "animation name: " << mAnimationName << std::endl;
            FbxTakeInfo* takeInfo = scene->GetTakeInfo( animStackName );
            FbxTime start = takeInfo->mLocalTimeSpan.GetStart();
            FbxTime end = takeInfo->mLocalTimeSpan.GetStop();
            /*mAnimationLength = end.GetFrameCount( FbxTime::eFrames24 ) - start.GetFrameCount( FbxTime::eFrames24 ) + 1;
            Keyframe** currAnim = &skeleton.joints[ currJointIndex ].mAnimation;

            for (FbxLongLong i = start.GetFrameCount( FbxTime::eFrames24 ); i <= end.GetFrameCount( FbxTime::eFrames24 ); ++i)
            {
                FbxTime currTime;
                currTime.SetFrame( i, FbxTime::eFrames24 );
                *currAnim = new Keyframe();
                (*currAnim)->mFrameNum = i;
                FbxAMatrix currentTransformOffset = inNode->EvaluateGlobalTransform( currTime ) * geometryTransform;
                (*currAnim)->mGlobalTransform = currentTransformOffset.Inverse() * currCluster->GetLink()->EvaluateGlobalTransform( currTime );
                currAnim = &((*currAnim)->mNext);
            }*/
        }
    }

    // Some of the control points only have less than 4 joints
    // affecting them.
    // For a normal renderer, there are usually 4 joints
    // I am adding more dummy joints if there isn't enough
    /*BlendingIndexWeightPair currBlendingIndexWeightPair;
    currBlendingIndexWeightPair.blendingIndex = 0;
    currBlendingIndexWeightPair.blendingWeight = 0;
    for (auto itr = mControlPoints.begin(); itr != mControlPoints.end(); ++itr)
    {
        for (unsigned int i = itr->second->mBlendingInfo.size(); i <= 4; ++i)
        {
            itr->second->mBlendingInfo.push_back( currBlendingIndexWeightPair );
        }
    }*/
}

void ProcessSkeletonHierarchyRecursively( const FbxNode* inNode, int myIndex, int inParentIndex )
{
    if (inNode->GetNodeAttribute() && inNode->GetNodeAttribute()->GetAttributeType() && inNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
    {
        Joint currJoint;
        currJoint.parentIndex = inParentIndex;
        currJoint.name = inNode->GetName();
        skeleton.joints.push_back( currJoint );
    }

    for (std::size_t childIndex = 0; childIndex < inNode->GetChildCount(); ++childIndex)
    {
        ProcessSkeletonHierarchyRecursively( inNode->GetChild( static_cast< int >( childIndex ) ), static_cast< int >( skeleton.joints.size() ), myIndex );
    }
}

void ProcessSkeletonHierarchy( const FbxNode* rootNode )
{
    for (int childIndex = 0; childIndex < rootNode->GetChildCount(); ++childIndex)
    {
        const FbxNode* currNode = rootNode->GetChild( childIndex );
        ProcessSkeletonHierarchyRecursively( currNode, 0, -1 );
    }
}

void LoadFBX( const std::string& path )
{
    std::ifstream ifs( path.c_str() );
    if (!ifs)
    {
        std::cerr << "Couldn't open file " << path << std::endl;
        exit( 1 );
    }

    const std::string extension = path.substr( path.length() - 3, path.length() );

    if (extension != "fbx" && extension != "FBX")
    {
        std::cerr << path << " is not .fbx!" << std::endl;
        exit( 1 );
    }

    FbxScene* const theScene = ImportScene( path );
    FbxNode* rootNode = theScene->GetRootNode();

    ImportMeshNodes( rootNode );

    /*ProcessSkeletonHierarchy( rootNode );
    ProcessJointsAndAnimations( rootNode );
    std::cout << "skeleton: " << std::endl;
    for (std::size_t i = 0; i < skeleton.joints.size(); ++i)
    {
        std::cout << "joint " << skeleton.joints[ i ].name << ", parent index "  << skeleton.joints[ i ].parentIndex << std::endl;
    }

    std::cout << "end" << std::endl;*/
}

int main( int paramCount, char** params )
{
    if (paramCount != 2)
    {
        std::cerr << "Usage: ./convert_fbx file.fbx" << std::endl;
        return 1;
    }

    std::ifstream ifs( params[ 1 ], std::ios_base::binary );

    if (!ifs)
    {
        std::cerr << "Couldn't open file " << params[ 1 ] << std::endl;
        return 1;
    }

    std::cerr << "Converting... " << std::endl;

    LoadFBX( params[ 1 ] );

    if (gMeshes.empty())
    {
        std::cout << std::string( params[ 1 ] ) << " didn't contain any meshes." << std::endl;
        return 0;
    }

    // Creates a new file name by replacing 'obj' with 'ae3d'.
    std::string outFile = std::string( params[ 1 ] );
    outFile = outFile.substr( 0, outFile.length() - 3 );
    outFile.append( "ae3d" );

    WriteAe3d( outFile, VertexFormat::PTNTC );
    return 0;
}
