bl_info = {
    'name': 'Export Aether3D model format (.ae3d)',
    'author': 'Timo Wiren',
    'version': (0,6),
    'blender': (2, 80, 0),
    'location': 'File > Export > Aether3D',
    'description': 'Exports selected meshes to the Aether3D Engine format (.ae3d)',
    "wiki_url": "https://github.com/bioglaze/aether3d",
    'category': 'Import-Export'}

# Skeleton/animation code based on https://gitlab.com/bztsrc/model3d/tree/master/blender/

import bpy
import bpy.props
import bpy.utils
import ctypes
import math
import mathutils
import os
import sys
import struct
from bpy.props import (
        BoolProperty,
        FloatProperty,
        StringProperty,
        IntProperty,
        EnumProperty,
        )

from bpy_extras.io_utils import axis_conversion;

class Vertex:
    co = mathutils.Vector()
    normal = mathutils.Vector()
    uv = []
    tangent = mathutils.Vector( [ 0.0, 0.0, 0.0, 0.0 ] )
    bitangent = mathutils.Vector()
    color = mathutils.Vector()

def uniquelist( l, e ):
    try:
        i = l.index( e )
    except ValueError:
        i = len( l )
        l.append( e )
    return i

def vert( x, y, z, w ):
    if x == -0.0:
        x = 0.0
    if y == -0.0:
        y = 0.0
    if z == -0.0:
        z = 0.0
    if w == -0.0:
        w = 0.0
    return x, y, z, w

class Mesh:
    vertices = []
    tangents = []
    faces = []
    name = "unnamed"
    aabbMin = [ 99999999.0, 99999999.0, 99999999.0 ]
    aabbMax = [-99999999.0,-99999999.0,-99999999.0 ]


    def generateAABB( self ):
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
    meshes = list()
    aabbMin = [ 99999999.0, 99999999.0, 99999999.0 ]
    aabbMax = [-99999999.0,-99999999.0,-99999999.0 ]

    exportSkeleton: BoolProperty(
            name="Write Armature",
            description="Write out armature (bone hierarchy and skin)",
            default=True,
            )

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
    
        ##  faulty (non-planar) faces may have a zero-length normal, and without
        ##  direction you can't calculate direction difference
        if fop.normal.length > 0.0:
            for p in mesh.polygons:
                if ( p != fop ) and ( mesh_vi in p.vertices ) and ( p.normal.length > 0.0):
                    angle = int( round( math.degrees( fop.normal.angle( p.normal ) ) ))
                    if  angle <= mesh.auto_smooth_angle:
                        result = mesh.vertices[ mesh_vi ].normal
        return result


    def get_vertex_pnt( self, obj_prop, mesh, face, face_vi ):
        # position
        co = obj_prop[ OBJ.LOC ] + mathutils.Vector( obj_prop[ OBJ.ROT ] @ mathutils.Vector([ \
                                                                                        mesh.vertices[ face.vertices[ face_vi ] ].co[ 0 ] * obj_prop[ OBJ.SCA ][ 0 ], \
                                                                                        mesh.vertices[ face.vertices[ face_vi ] ].co[ 1 ] * obj_prop[ OBJ.SCA ][ 1 ], \
                                                                                        mesh.vertices[ face.vertices[ face_vi ] ].co[ 2 ] * obj_prop[ OBJ.SCA ][ 2 ] \
                                                                                        ]))
        # normal
        if face.use_smooth:
            if mesh.use_auto_smooth:
                no = mathutils.Vector( obj_prop[ OBJ.ROT ] @ get_autosmooth_normal( mesh, face, face.vertices[ face_vi ] ))
            else:
                no = mathutils.Vector( obj_prop[ OBJ.ROT ] @ mesh.vertices[ face.vertices[ face_vi ] ].normal )
        else:
                no = mathutils.Vector( obj_prop[ OBJ.ROT ] @ face.normal )

        color = ( 1.0, 1.0, 1.0, 1.0 )

        outVertex = Vertex()
        outVertex.co = co
        outVertex.normal = no
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

        # FIXME: This should be read from the export property.
        exportSkeleton = True
        
        for obj in objects:
            mesh = Mesh()
            mesh.vertices = []
            mesh.faces = []
            mesh.name = obj.name
            
            obj.data.calc_loop_triangles()
            obj.data.calc_tangents()
            
            object[ OBJ.MAT ] = obj.matrix_world.copy() @ mathutils.Matrix.Rotation(-math.pi/2, 4, 'X')
            object[ OBJ.LOC ] = object[ OBJ.MAT ].to_translation()
            object[ OBJ.ROT ] = object[ OBJ.MAT ].to_quaternion()
            object[ OBJ.SCA ] = object[ OBJ.MAT ].to_scale()
            object[ OBJ.UVL ] = None

            armatures = [Object for Object in context.selected_objects if Object.type in ("ARMATURE")]
            global_matrix = axis_conversion( to_forward='Z', to_up='Y' ).to_4x4()
            armOffs = 0
            bones = []
            boneNames = []
            verts = []

            for armature in armatures:
                for bone in armature.data.bones:
                    print( "bone " + bone.name )
                    m = global_matrix @ bone.matrix_local
                    a = -1
                
                    if bone.parent:
                        for j, p in enumerate( armature.data.bones ):
                            if p == bone.parent:
                                a = j + armOffs
                                break;
                        pos = global_matrix @ bone.parent.matrix_local
                        m = pos.inverted() @ m
                    pos = m.to_translation()
                    q = m.to_quaternion()
                    q.normalize()
                    # n = safestr(b.name)
                    try:
                        boneNames.index( n )
                        print( "bone name not unique: " + bone.name )
                        break
                    except:
                        pass

                    digits = 4
                    bones.append([ a, uniquelist( boneNames, bone.name ),
                        uniquelist(verts, [vert(
                            round( pos[0], digits),
                            round( pos[1], digits),
                            round( pos[2], digits), 1.0), 0, -1]),
                        uniquelist(verts, [vert(
                            round(q.x, digits),
                            round(q.y, digits),
                            round(q.z, digits),
                            round(q.w, digits)), 0, -2])])
                armoffs = len( bones )
                print("armoffs: ", armoffs)

            for uvt in obj.data.uv_layers:
                if uvt.active_render:
                    object[ OBJ.UVL ] = uvt.data

            f = 0
            
            for face in obj.data.loop_triangles:
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

            u = 0

            for uv_layer in obj.data.uv_layers:
                for tri in obj.data.loop_triangles:
                    for loop_index in tri.loops:
                        mesh.vertices[ u ].uv = uv_layer.data[ loop_index ].uv
                        u = u + 1

            u = 0
            for p in obj.data.loops:
                mesh.vertices[ u ].tangent = (p.tangent.x, p.tangent.y, p.tangent.z, p.bitangent_sign )
                u = u + 1

            u = 0
            for cc in obj.data.vertex_colors:
                for c in cc.data:
                    mesh.vertices[ u ].color = c.color
                    u = u + 1

            print( "vertex groups: " + str( len( obj.vertex_groups ) ) )
            if exportSkeleton and len( obj.vertex_groups ) > 0:
                vg = obj.vertex_groups
            else:
                vg = []
                
            mesh.generateAABB()
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
    bpy.utils.register_class( Aether3DExporter )
    bpy.types.TOPBAR_MT_file_export.append( menu_func )


def unregister():
    bpy.utils.unregister_class( Aether3DExporter )
    bpy.types.TOPBAR_MT_file_export.remove( menu_func )


if __name__ == "__main__":
    register()
