bl_info = {
    'name': 'Export Aether3D model format (.ae3d)',
    'author': 'Timo Wiren',
    'version': (0,5),
    'blender': (2, 6, 8),
    "api": 35622,
    'location': 'File > Export',
    'description': 'Exports selected meshes to the Aether3D Model Format (.ae3d)',
    'category': 'Import-Export'}

import bpy
import bpy.props
import bpy.utils
import os
import sys
import struct
import math
import mathutils
import ctypes

class Vertex:
    co = mathutils.Vector()
    normal = mathutils.Vector()
    uv = []
    tangent = mathutils.Vector( [ 0.0, 0.0, 0.0, 0.0 ] )
    bitangent = mathutils.Vector()
    color = mathutils.Vector()


class Mesh:
    vertices = []
    tangents = []
    faces = []
    name = "unnamed"
    aabbMin = [ 99999999.0, 99999999.0, 99999999.0 ]
    aabbMax = [-99999999.0,-99999999.0,-99999999.0 ]


    def generateAABB( self ):
        #print( "" + self.name )
        #print( "aabbMin: " + str(self.aabbMin[0]) + ", " + str( self.aabbMin[1] ) + ", " + str(self.aabbMin[2]) )

        self.aabbMin = [ 99999999.0, 99999999.0, 99999999.0 ]
        self.aabbMax = [-99999999.0,-99999999.0,-99999999.0 ]

        for v in self.vertices:
            if self.aabbMin[0] > v.co[0]:
                self.aabbMin[0] = v.co[0]
        
            if self.aabbMin[1] > v.co[1]:
                self.aabbMin[1] = v.co[1]
        
            if self.aabbMin[2] > v.co[2]:
                self.aabbMin[2] = v.co[2]
        
            if self.aabbMax[0] < v.co[0]:
                self.aabbMax[0] = v.co[0]
        
            if self.aabbMax[1] < v.co[1]:
                self.aabbMax[1] = v.co[1]
        
            if self.aabbMax[2] < v.co[2]:
                self.aabbMax[2] = v.co[2]


    def generateTangents( self ):
        # Face tangents.
        faceTangents = []
        faceBitangents = []

        for f in range( int( len( self.faces ) ) ):
            uv1 = self.vertices[ self.faces[ f ][ 0 ] ].uv
            uv2 = self.vertices[ self.faces[ f ][ 1 ] ].uv
            uv3 = self.vertices[ self.faces[ f ][ 2 ] ].uv

            deltaU1 = uv2[ 0 ] - uv1[ 0 ]
            deltaU2 = uv3[ 0 ] - uv1[ 0 ]

            deltaV1 = uv2[ 1 ] - uv1[ 1 ]
            deltaV2 = uv3[ 1 ] - uv1[ 1 ]

            area = deltaU1 * deltaV2 - deltaV1 * deltaU2;

            pos1 = self.vertices[ self.faces[ f ][ 0 ] ].co
            pos2 = self.vertices[ self.faces[ f ][ 1 ] ].co
            pos3 = self.vertices[ self.faces[ f ][ 2 ] ].co

            deltaPos1 = pos2 - pos1
            deltaPos2 = pos3 - pos1

            tangent = mathutils.Vector( [ 1.0, 0.0, 0.0, 0.0 ])
            bitangent = mathutils.Vector( [ 0.0, 1.0, 0.0, 0.0 ])

            if abs( area ) > 0.0001:
                tangent   = (deltaPos1 * deltaV2 - deltaPos2 * deltaV1) / area;
                bitangent = (deltaPos2 * deltaU1 - deltaPos1 * deltaU2) / area;
                tangent = mathutils.Vector( [ tangent.x, tangent.y, tangent.z, 0.0 ] )
                bitangent = mathutils.Vector( [ bitangent.x, bitangent.y, bitangent.z, 0.0 ] )

            faceTangents.append( tangent )
            faceBitangents.append( bitangent )

        # Vertex tangents are averages of face tangents.

        for v in range( int( len( self.vertices ) ) ): 
            self.vertices[ v ].tangent = mathutils.Vector( [ 0.0, 0.0, 0.0, 0.0 ] )

        for v in range( int( len( self.vertices ) ) ): 
            tangent = mathutils.Vector( [ 0.0, 0.0, 0.0, 0.0 ] )
            bitangent = mathutils.Vector( [ 0.0, 0.0, 0.0, 0.0 ] )

            for f in range( int( len( self.faces ) ) ):
                if self.faces[ f ][ 0 ] == v or self.faces[ f ][ 1 ] == v or self.faces[ f ][ 2 ] == v:
                    tangent += faceTangents[ f ]
                    bitangent += faceBitangents[ f ]

            self.vertices[ v ].tangent = tangent
            self.vertices[ v ].bitangent = bitangent

        for v in range( int( len( self.vertices ) ) ): 
            normal = self.vertices[ v ].normal.copy()
            normal3 = mathutils.Vector( [ normal.x, normal.y, normal.z ] )

            tangent = self.vertices[ v ].tangent.copy()
            tangent3 = mathutils.Vector( [ tangent.x, tangent.y, tangent.z ] )
            
            # Gram-Schmidt orthonormalization.
            tangent3 -= normal3 * normal3.dot( tangent3 )
            self.vertices[ v ].tangent = mathutils.Vector( [ tangent3.x, tangent3.y, tangent3.z, 0.0 ])
            self.vertices[ v ].tangent.normalize()

            # Handedness.
            cp = normal3.cross( tangent3 )
            self.vertices[ v ].tangent[ 3 ] = 1.0 if cp.dot( self.vertices[v].bitangent ) >= 0.0 else -1.0;


# object data dictionary keys
class OBJ:
    MAT = 'm'  # possibly scaled object matrix
    LOC = 'l'  # object location
    ROT = 'r'  # object rotation
    SCA = 's'  # object scale
    UVL = 'u'  # active uv-layer


class Aether3DExporter( bpy.types.Operator ):
    """Export to the Aether3D model format (.ae3d)"""
    
    bl_idname = "export.aether3d"
    bl_description = "Exports selected meshes into Aether3D .ae3d format."
    bl_label = "Export Aether3D"
    
    filepath = bpy.props.StringProperty()
    filename = bpy.props.StringProperty()
    directory = bpy.props.StringProperty()
    meshes = list()
    aabbMin = [ 99999999.0, 99999999.0, 99999999.0 ]
    aabbMax = [-99999999.0,-99999999.0,-99999999.0 ]


    def generateAABB( self ):
        for m in self.meshes:
            if self.aabbMin[0] > m.aabbMin[0]:
                self.aabbMin[0] = m.aabbMin[0]
        
            if self.aabbMin[1] > m.aabbMin[1]:
                self.aabbMin[1] = m.aabbMin[1]
        
            if self.aabbMin[2] > m.aabbMin[2]:
                self.aabbMin[2] = m.aabbMin[2]
        
            if self.aabbMax[0] < m.aabbMax[0]:
                self.aabbMax[0] = m.aabbMax[0]
        
            if self.aabbMax[1] < m.aabbMax[1]:
                self.aabbMax[1] = m.aabbMax[1]
        
            if self.aabbMax[2] < m.aabbMax[2]:
                self.aabbMax[2] = m.aabbMax[2]


    def get_autosmooth_normal(mesh, fop, mesh_vi):
        """check if smoothing has to be applied and return the depending normal"""
        ##  is at least one neighbourface smooth, the we return the vertex
        ##  normal(smooth applied), else the face normal will be returned
        result = fop.normal  # init with the normal for the un-smooth case
    
        ##  faulty (none-planar) faces may have a zero-length normal, and without
        ##  direction you can't calculate direction difference
        if fop.normal.length > 0.0:
            for p in mesh.polygons:
                if ( p != fop ) and ( mesh_vi in p.vertices ) and ( p.normal.length > 0.0):
                    angle = int(round(math.degrees(fop.normal.angle(p.normal))))
                    if  angle <= mesh.auto_smooth_angle:
                        result = mesh.vertices[mesh_vi].normal
        return result


    def get_vertex_pnt( self, obj_prop, mesh, face, face_vi ):
        # position
        co = obj_prop[ OBJ.LOC ] + mathutils.Vector(obj_prop[OBJ.ROT] * mathutils.Vector([ \
                                                                                        mesh.vertices[face.vertices[face_vi]].co[0] * obj_prop[OBJ.SCA][0], \
                                                                                        mesh.vertices[face.vertices[face_vi]].co[1] * obj_prop[OBJ.SCA][1], \
                                                                                        mesh.vertices[face.vertices[face_vi]].co[2] * obj_prop[OBJ.SCA][2] \
                                                                                        ]))
        # normal
        if face.use_smooth:
            if mesh.use_auto_smooth:
                no = mathutils.Vector(obj_prop[OBJ.ROT] * get_autosmooth_normal(mesh, face, face.vertices[face_vi]))
            else:
                no = mathutils.Vector(obj_prop[OBJ.ROT] * mesh.vertices[face.vertices[face_vi]].normal)
        else:
                no = mathutils.Vector(obj_prop[OBJ.ROT] * face.normal)

        # texture coords
        if obj_prop[ OBJ.UVL ] != None:
            uv = (obj_prop[OBJ.UVL][face.index].uv[face_vi][0], \
                  1 - obj_prop[OBJ.UVL][face.index].uv[face_vi][1])
        else:
            uv = (0.0, 0.0)

        # color
        if len( mesh.tessface_vertex_colors ) > 0:
            vcolors = mesh.tessface_vertex_colors[ 0 ]
            color = ( 1.0, 1.0, 1.0, 1.0 )

            n = len(vcolors.data)

            if face_vi == 0:
                vcolors1 = (ctypes.c_float * (n * 3))()
                vcolors.data.foreach_get( 'color1', vcolors1 )
                color = (vcolors1[0], vcolors1[1], vcolors1[2], 1.0)
            if face_vi == 1:
                vcolors2 = (ctypes.c_float * (n * 3))()
                vcolors.data.foreach_get( 'color2', vcolors2 )
                color = (vcolors2[0], vcolors2[1], vcolors2[2], 1.0)
            if face_vi == 2:
                vcolors3 = (ctypes.c_float * (n * 3))()
                vcolors.data.foreach_get( 'color3', vcolors3 )
                color = (vcolors3[0], vcolors3[1], vcolors3[2], 1.0)
            if face_vi == 3:
                vcolors4 = (ctypes.c_float * (n * 3))()
                vcolors.data.foreach_get( 'color4', vcolors4 )
                color = (vcolors4[0], vcolors4[1], vcolors4[2], 1.0)
        else:
            color = ( 1.0, 1.0, 1.0, 1.0 )

        # tangent
        #print( "tangent: " + str( mesh.tangent_space.face[ face.index ].vertices[ face_vi ].tangent ) )
        #print( "tangent " + str( face.tangent ))
        #print( "tangent " + str( mesh.vertices[ face.vertices[face_vi] ].tangent )) # mesh.vertices is MeshVertex        
        
        outVertex = Vertex()
        outVertex.co = co
        outVertex.normal = no
        outVertex.uv = uv
        outVertex.color = color
        outVertex.tangent = (1.0, 0.0, 0.0, 1.0)

        return outVertex
            

    def get_override(self, area_type, region_type):
        for area in bpy.context.screen.areas:
            if area.type == area_type:
                for region in area.regions:
                    if region.type == region_type:
                        override = {'area': area, 'region': region}
                        return override
        #error message if the area or region wasn't found
        raise RuntimeError("Wasn't able to find", region_type," in area ", area_type, "\n Make sure it's open while executing script.")
                        
    def readMeshes( self, context ):
        """Reads meshes."""
        
        self.meshes = []
        objects = [Object for Object in context.selected_objects if Object.type in ("MESH")]
        object = {}
        
        if len( objects ) == 0:
            self.report({'WARNING'}, "No meshes selected!")

        override = self.get_override( 'VIEW_3D', 'WINDOW' )
        #rotate about the X-axis by 45 degrees
        #bpy.ops.transform.rotate(override, value=6.283/8, axis=(1,0,0))
        bpy.ops.transform.rotate(override, value=-math.pi/2, axis=(1,0,0))
    
        for obj in objects:
            mesh = Mesh()
            mesh.vertices = []
            mesh.faces = []
            mesh.name = obj.name
            
            obj.data.calc_tessface()
            obj.data.calc_tangents()
            
            object[ OBJ.MAT ] = obj.matrix_world.copy()
            object[ OBJ.LOC ] = object[ OBJ.MAT ].to_translation()
            object[ OBJ.ROT ] = object[ OBJ.MAT ].to_quaternion()
            object[ OBJ.SCA ] = object[ OBJ.MAT ].to_scale()
            object[ OBJ.UVL ] = None

            for uvt in obj.data.tessface_uv_textures:
                if uvt.active_render:
                    object[ OBJ.UVL ] = uvt.data

            f = 0

            for face in obj.data.tessfaces:
                for vertex_id in (0, 1, 2):
                    vertex_pnt = self.get_vertex_pnt(object, obj.data, face, vertex_id)
                    mesh.vertices.append( vertex_pnt )
                tri = [ f * 3 + 0, f * 3 + 1, f * 3 + 2 ]
                mesh.faces.append( tri )
                f = f + 1
                
                ## check for and export a second triangle
                if len(face.vertices) == 4:
                    for vertex_id in (0, 2, 3):
                        vertex_pnt = self.get_vertex_pnt(object, obj.data, face, vertex_id)
                        mesh.vertices.append( vertex_pnt )
                        tri = [ f * 3 + 0, f * 3 + 1, f * 3 + 2 ]

                    mesh.faces.append( tri )
                    f = f + 1
            mesh.generateAABB()
            mesh.generateTangents()
            self.meshes.append( mesh )


    def writeFile( self, context, FilePath ):
        """Writes selected meshes to .ae3d file."""

        f = open( FilePath, "wb" )
        
        # Writes header.
        data1 = struct.pack( 'b', 97 ) # 'a'
        data2 = struct.pack( 'b', 57 ) # '9'
        f.write( data1 )
        f.write( data2 )
        
        # Model's AABB.
        component = struct.pack( 'f', self.aabbMin[0] )
        f.write( component )
        component = struct.pack( 'f', self.aabbMin[1] )
        f.write( component )
        component = struct.pack( 'f', self.aabbMin[2] )
        f.write( component )

        component = struct.pack( 'f', self.aabbMax[0] )
        f.write( component )
        component = struct.pack( 'f', self.aabbMax[1] )
        f.write( component )
        component = struct.pack( 'f', self.aabbMax[2] )
        f.write( component )

        # Mesh count, 2 bytes.
        nMeshes = struct.pack( 'H', len( self.meshes ) )
        f.write( nMeshes )

        for mesh in self.meshes:
            # Writes mesh's AABB min and max.
            component = struct.pack( 'f', mesh.aabbMin[0] )
            f.write( component )
            component = struct.pack( 'f', mesh.aabbMin[1] )
            f.write( component )
            component = struct.pack( 'f', mesh.aabbMin[2] )
            f.write( component )
    
            #print( "min: " + str( mesh.aabbMin[0] ) + ", " + str( mesh.aabbMin[1] ) + ", " + str( mesh.aabbMin[2] ) )

            component = struct.pack( 'f', mesh.aabbMax[0] )
            f.write( component )
            component = struct.pack( 'f', mesh.aabbMax[1] )
            f.write( component )
            component = struct.pack( 'f', mesh.aabbMax[2] )
            f.write( component )

            # Writes name length.
            nName = struct.pack( 'H', len( mesh.name ) )
            f.write( nName )

            # Writes name.
            f.write( mesh.name.encode() )
    
            # Writes # of vertices.
            nVertices = struct.pack( 'H', len( mesh.vertices ) )
            f.write( nVertices )

            # Writes vertex format. Currently hard-coded to PTNTC.
            vertexFormat = 0;
            vertexFormatPacked = struct.pack( 'b', vertexFormat )
            f.write( vertexFormatPacked )

            # Writes vertices.
            for v in mesh.vertices:
                # Position.
                component = struct.pack( 'f', v.co[ 0 ] )
                f.write( component )
                component = struct.pack( 'f', v.co[ 1 ] )
                f.write( component )
                component = struct.pack( 'f', v.co[ 2 ] )
                f.write( component )
    
                # Texture coordinate.
                s = v.uv[ 0 ]
                t = v.uv[ 1 ]
    
                component = struct.pack( 'f', s )
                f.write( component )
                component = struct.pack( 'f', t )
                f.write( component )
    
                # Normal.
                component = struct.pack( 'f', v.normal[ 0 ] )
                f.write( component )
                component = struct.pack( 'f', v.normal[ 1 ] )
                f.write( component )
                component = struct.pack( 'f', v.normal[ 2 ] )
                f.write( component )
    
                # Tangent.
                component = struct.pack( 'f', v.tangent[ 0 ] )
                f.write( component )
                component = struct.pack( 'f', v.tangent[ 1 ] )
                f.write( component )
                component = struct.pack( 'f', v.tangent[ 2 ] )
                f.write( component )
                component = struct.pack( 'f', v.tangent[ 3 ] )
                f.write( component )

                # Vertex color.
                component = struct.pack( 'f', v.color[ 0 ] )
                f.write( component )
                component = struct.pack( 'f', v.color[ 1 ] )
                f.write( component )
                component = struct.pack( 'f', v.color[ 2 ] )
                f.write( component )
                component = struct.pack( 'f', v.color[ 3 ] )
                f.write( component )

            # Writes # of faces.
            nFaces = struct.pack( 'H', int( len( mesh.faces ) ) )
            f.write( nFaces )

            # Writes faces.
            for i in mesh.faces:
                component = struct.pack( 'H', i[0] )
                f.write( component )
                component = struct.pack( 'H', i[1] )
                f.write( component )
                component = struct.pack( 'H', i[2] )
                f.write( component )

        # Terminator
        terminator = 100
        o = struct.pack( 'b', int( terminator ) )
        f.write( o )
        f.close()


    def execute( self, context ):
        FilePath = self.properties.filepath
        if not FilePath.lower().endswith( ".ae3d" ):
            FilePath += ".ae3d"

        self.readMeshes( context )
        self.generateAABB()
        self.writeFile( context, FilePath )
        self.report({'INFO'}, "Export finished successfully." )
        return {"FINISHED"}


    def invoke( self, context, event ):
        WindowManager = context.window_manager
        WindowManager.fileselect_add( self )
        return {"RUNNING_MODAL"}

## register script as operator and add it to the File->Export menu

def menu_func( self, context ):
    defaultPath = os.path.splitext( bpy.data.filepath)[0] + ".ae3d"
    self.layout.operator(Aether3DExporter.bl_idname, text="Aether3D (.ae3d)").filepath = defaultPath

def register():
    bpy.utils.register_module( __name__ )
    bpy.types.INFO_MT_file_export.append( menu_func )


def unregister():
    bpy.utils.unregister_module( __name__ )
    bpy.types.INFO_MT_file_export.remove( menu_func )


if __name__ == "__main__":
    register()
