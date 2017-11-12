cd ../../../aether3d_build/Samples

for f in *.fsh
do 
    cp -v $f $f.frag
    ~/Downloads/VulkanSDK/1.0.65.0/x86_64/bin/glslangValidator $f.frag
    rm $f.frag
done

for f in *.vsh
do 
    cp -v $f $f.vert
    ~/Downloads/VulkanSDK/1.0.65.0/x86_64/bin/glslangValidator $f.vert
    rm $f.vert
done
