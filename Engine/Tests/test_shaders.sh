cd ../../../aether3d_build/Samples

for f in *.fsh
do 
    cp -v $f $f.frag
    ~/Downloads/glslangValidator $f.frag
    rm $f.frag
done

for f in *.vsh
do 
    cp -v $f $f.vert
    ~/Downloads/glslangValidator $f.vert
    rm $f.vert
done
