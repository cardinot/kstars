 #!/bin/bash
 export KSTARS_DIR=~/kde/install
 export PATH=$KSTARS_DIR/bin:$PATH
 export LD_LIBRARY_PATH=$KSTARS_DIR/lib:$LD_LIBRARY_PATH
 export KDEDIR=$KSTARS_DIR
 export KDEDIRS=$KDEDIR
 export XDG_DATA_DIRS=$XDG_DATA_DIRS:$KSTARS_DIR/share
 # update KDE's system configuration cache
 kbuildsycoca4
 # start app
 /home/cardinot/kde/kstars/build/kstars/./kstars
