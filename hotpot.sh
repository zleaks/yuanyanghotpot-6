#!/bin/bash
#set -x

function usage() {
    cat << EOF
用法：
    sudo ./hotpot.sh action [option]

    action:
        make                 增量编译（依次执行make和install）
        remake               全量编译（依次执行configure、clean、make和install）
        install              重新安装
        deb                  火锅打包
        appmanager           编译wine-appmanager
        --help               帮助
        --about              关于设计说明

    option:
        -i, --install=INSTALL_DIR | no    （INSTALL_DIR 默认为工程目录的install目录）
        -c, --cflags=CPPFLAGS             （CPPFLAGS 默认为__HOTPOT_DISABLE_REGISTER__ 和 __HOTPOT_PROFESSIONAL__）
        -a, --apps=APPS                   （APPS,多个app用,分割 默认为tools/scripts/hotpot_apps.conf内定义的应用）
        --only-32                          只编译32位
        -v, --version=versionNumber        打包版本号为versionNumber的deb文件

例子：
    --install=/  ||  -i /
    --cflags=__HOTPOT_DISABLE_REGISTER__,__HOTPOT_PROFESSIONAL__ || -c __HOTPOT_DISABLE_REGISTER__,__HOTPOT_NORMAL__
    --apps=hotpot,wechat  ||  -a hotpot,wechat,qq,wemeet
    --only-32
    --version=6.0.5.23-p-noreg-64combo  ||  -v 6.0.5.23-p-noreg-64combo
EOF
    exit
}

function about() {
    cat << EOF
编译过程：
    1、在~/workspace/.arch目录中，准备两个debian9的子系统"debian9_i386"、"debian9_amd64"用于编译；
    2、在hotpot_apps.conf内定义了默认参加编译的应用：
        例如：wechat=(wechat-win-u "dlls/gdi32 dlls/gdiplus dlls/user32 dlls/winex11.drv" 32 __HOTPOT_WECHAT__)
    3、本脚本的上级目录是工程目录，它将被挂载到子系统的root目录，用来执行编译操作
    4、执行编译后，会在code目录的同级创建一个out目录，用来存放编译生成的一切文件；
        out目录里又创建N组编译目录(如hotpot,wechat,catia)和一个install目录
        每组编译目录里，创建三个编译目录(build32,build64,build_combo)，下面举个列子：

                                                                       |--- build32
                        |--- hotpot6.0_p ---|--- out ---|--- hotpot ---|--- build64
                        |                   |           |              |--- build_combo
                        |                   |           |
                        |                   |           |              |--- build32
                        |                   |           |--- wechat ---|--- build64
                        |                   |           |              |--- build_combo
                        |                   |           |
                        |                   |           |              |--- build32
                        |                   |           |--- catiav5---|--- build64
                        |                   |           |              |--- build_combo
                        |                   |           |--- install
                        |                   |
        ~/workspace  ---|                   |--- code---|--- hotpot.sh
                        |
                        |--- hotpot6.0_u ---|--- ......目录结构同上
                        |
                        |可以有多个工程目录 | out和code|多个app和install| 三个build目录 
                        |
                        |                   |--- debian9_i386
                        |--- .arch       ---|--- debian9_amd64
                        |
                        |--- yuanyanghotpot_deb_maker_6.0

安装过程：
    1、hotpot目录编译成功后，默认安装到out/install目录，也可以通过 -i 指定安装目录；
    2、app(如wechat)目录，编译成功后，将本app修改生成的so文件，拷贝到/usr/local/share/wine/dlls/appname(如wechat-win-u)目录；

执行过程：
    1、在应用启动的时候，优先在/usr/local/share/wine/dlls/appname(如wechat-win-u)目录加载库文件
EOF
    exit
}

if [ "$1" == "--help" ] || [ -z "$1" ]; then usage; fi
if [ "$1" == "--about" ]; then about; fi

startTime=`date +%Y%m%d-%H:%M:%S`
startTime_s=`date +%s`

CODE_DIR=$(basename $(pwd))
BUILDING_WORKSPACE_PATH=$(dirname $(pwd))
HOME_DIR=/home/$(logname)/workspace
CHROOT_I386=$HOME_DIR/.arch/debian9_i386
CHROOT_AMD64=$HOME_DIR/.arch/debian9_amd64
BUILD_32_DIR_NAME=build32
BUILD_64_DIR_NAME=build64
BUILD_32_64_DIR_NAME=build_combo
# DEB_PATH=$HOME_DIR/yuanyanghotpot_deb_maker_6.0
DEB_PATH=$BUILDING_WORKSPACE_PATH/out/deb_maker
WINE_ROOT="/opt/winux/hotpot6/usr/local/"

PARAM_INSTALL_DIR=""
PARAM_CONFIG_FLAGS=()
PARAM_APPS=()
ONLY32=0
DEB_VERSION=""
export CPP_FLAGS=""

source tools/scripts/hotpot_apps_u.conf
building_apps_u=($(egrep '^[a-z_0-9]+=\(' tools/scripts/hotpot_apps_u.conf | awk -F "=" '{print $1}' | xargs))
source tools/scripts/hotpot_apps_p.conf
building_apps_p=($(egrep '^[a-z_0-9]+=\(' tools/scripts/hotpot_apps_p.conf | awk -F "=" '{print $1}' | xargs))

function parse_params() {
    OPT_PARAM=$(getopt -u -o i:c:a:v: -al install:,cflags:,apps:,only-32,version: "$@")
    exit_when_error $1 $?

    set -- $OPT_PARAM

    while [ -n "$1" ]
    do
        case "$1" in
            -i|--install)
            {
                PARAM_INSTALL_DIR=$2
                shift 2
            }
            ;;
            -c|--cflags)
            {
                local str=-D$2
                PARAM_CONFIG_FLAGS=(${str//,/ -D})
                shift 2
            }
            ;;
            -a|--apps)
            {
                local str=$2
                PARAM_APPS=(${str//,/ })
                shift 2
            }
            ;;
            --only-32)
            {
                ONLY32=1
                shift 1
            }
            ;;
            -v|--version)
            {
                DEB_VERSION=$2
                shift 2
            }
            ;;
            --) break ;;
        esac
    done

    if [ ${#PARAM_CONFIG_FLAGS[@]} == 0 ]; then
        PARAM_CONFIG_FLAGS=(-D__HOTPOT_DISABLE_REGISTER__ -D__HOTPOT_PROFESSIONAL__)
    fi

    if [ ${#PARAM_APPS[@]} == 0 ]; then
        local is_professional=0
        for flag in ${PARAM_CONFIG_FLAGS[@]}; do
            if [ "$flag" == "-D__HOTPOT_PROFESSIONAL__" ]; then
                is_professional=1
                break;
            fi
        done

        if [ "$is_professional" == "1" ]; then
            PARAM_APPS=(${building_apps_u[@]} ${building_apps_p[@]} hotpot)
        else
            PARAM_APPS=(${building_apps_u[@]} hotpot)
        fi
    fi
}

function do_umount() {
    mountpoint -q $1
    while [ $? == 0 ]
    do
        sudo umount $1
        if [ $? != 0 ]; then
            echo $1" had be mounted, and cann't be umounted"
            exit
        fi
        mountpoint -q $1
    done
}

function umount_workspace() {
    do_umount $CHROOT_I386/root
    do_umount $CHROOT_AMD64/root
}

function do_mount() {
    do_umount $1

    sudo mount -o bind $BUILDING_WORKSPACE_PATH $1
    if [ $? != 0 ]; then
        echo $1" cann't be mounted"
        exit
    fi
}

function mount_workspace() {
    do_mount $CHROOT_I386/root
    do_mount $CHROOT_AMD64/root
}

function make_project() {
    if [ "$3" != "hotpot" ]; then
        eval local modules=\${$3[$module_idx]}
        for module in $modules; do
            make_app ${@:1:4} $module
        done
        return
    fi

    sudo chroot $2 /bin/bash -x -e <<EOF
        cd /root/out/$3/$4/
        make -j$(nproc) &>> /root/out/log
EOF

    exit_when_error $1 $?
}

function make_app() {
    eval local arch=\${$3[$arch_idx]}
    if [ "$arch" == "32" ] && [ "$4" != "$BUILD_32_DIR_NAME" ]; then return; fi
    if [ "$arch" == "64" ] && [ "$ONLY32" == "1" ]; then return; fi
    if [ "$arch" == "64" ] && [ "$4" != "$BUILD_64_DIR_NAME" ]; then return; fi

    if [ "$3" == "finereader15" ] && [ "$5" == "dlls/comctl32" ]; then
        cd $BUILDING_WORKSPACE_PATH/$CODE_DIR/
        git apply tools/scripts/comctl32_v6.patch
    fi

    sudo chroot $2 /bin/bash -x -e <<EOF
        cd /root/out/$3/$4/
        make -C $5 -j$(nproc) &>> /root/out/log
EOF

    if [ "$3" == "finereader15" ] && [ "$5" == "dlls/comctl32" ]; then
        cd $BUILDING_WORKSPACE_PATH/$CODE_DIR/
        git checkout dlls/comctl32/comctl32.h
        git checkout dlls/comctl32/comctl32.rc
        git checkout include/commctrl.h
    fi

    exit_when_error $1 $?
}

function remake_project() {
    local flag_arr=${PARAM_CONFIG_FLAGS[@]}
    if [ "$3" != "hotpot" ]; then
        eval local arch=\${$3[$arch_idx]}
        if [ "$arch" == "32" ] && [ "$4" != "$BUILD_32_DIR_NAME" ]; then return; fi
        if [ "$arch" == "64" ] && [ "$ONLY32" == "1" ]; then return; fi
        if [ "$arch" == "64" ] && [ "$4" != "$BUILD_64_DIR_NAME" ]; then return; fi
        eval local macro=\${$3[$macro_idx]}
        flag_arr[${#flag_arr[@]}]=-D$macro
    fi
    CPP_FLAGS=${flag_arr[@]}
    sudo chroot $2 /bin/bash -x -e <<EOF
        rm -rf /root/out/$3/$4/ && mkdir -p /root/out/$3/$4/ && cd /root/out/$3/$4/
        CPPFLAGS="$CPP_FLAGS" /root/$CODE_DIR/configure --prefix=$WINE_ROOT ${@:5} &>> /root/out/log
        if [ "$3" == "hotpot" ]; then
            make -j$(nproc) &>> /root/out/log
        fi
EOF
    exit_when_error $1 $?

    if [ "$3" != "hotpot" ]; then
        make_project ${@:1:4}
    fi

    echo ""
}

function do_install_app() {
    eval local name=\${$1[$name_idx]}
    local share_dll_dir=$BUILDING_WORKSPACE_PATH/out/install/$WINE_ROOT/share/wine/dlls/$name
    eval local modules=\${$1[$module_idx]}
    eval local arch=\${$1[$arch_idx]}
    
    mkdir -p $share_dll_dir

    if [ "$arch" == "32" ]; then
        cd $BUILDING_WORKSPACE_PATH/out/$1/$BUILD_32_DIR_NAME/
    else
        cd $BUILDING_WORKSPACE_PATH/out/$1/$BUILD_64_DIR_NAME/
    fi

    for module in $modules; do
        find $module -name *.so | xargs -i cp {} $share_dll_dir
    done
}

function do_install() {
    if [ "$2" == "hotpot" ]; then
        sudo chroot $3 /bin/bash -x -e <<EOF
            mkdir -p /root/out/install
            cd /root/out/$2/$4/
            make install DESTDIR=../../install -j$(nproc) &>> /root/out/log
EOF
    else
        do_install_app $2
    fi

    exit_when_error $1 $?
}

function install_project() {
    if [ -d $BUILDING_WORKSPACE_PATH/out/install ];then
        sudo rm -rf $BUILDING_WORKSPACE_PATH/out/install
    fi

    if [ "$ONLY32" == "1" ]; then
        do_install $1 hotpot $CHROOT_I386 $BUILD_32_DIR_NAME
    else
        do_install $1 hotpot $CHROOT_I386 $BUILD_32_64_DIR_NAME
        do_install $1 hotpot $CHROOT_AMD64 $BUILD_64_DIR_NAME
    fi

    sudo rm -rf $BUILDING_WORKSPACE_PATH/out/install/$WINE_ROOT/share/wine/dlls
    for app in ${PARAM_APPS[@]}; do
        if [ "$app" != "hotpot" ]; then
            do_install $1 $app;
        fi
    done

    # sudo rm -rf $BUILDING_WORKSPACE_PATH/out/install/$WINE_ROOT/share/wine/regs
    # sudo cp -r $BUILDING_WORKSPACE_PATH/$CODE_DIR/tools/scripts/appmanager/regs $BUILDING_WORKSPACE_PATH/out/install/$WINE_ROOT/share/wine/

    if [ "$PARAM_INSTALL_DIR" != "" ]; then
        if [ -d $PARAM_INSTALL_DIR/$WINE_ROOT/share/wine/dlls ];then
            sudo rm -rf $PARAM_INSTALL_DIR/$WINE_ROOT/share/wine/dlls
        fi
        sudo cp -af $BUILDING_WORKSPACE_PATH/out/install/usr $PARAM_INSTALL_DIR
        exit_when_error $1 $?
    fi
}

function format_time() {
    local seconds=$1
    local hour=$(($seconds/3600))
    local min=$((($seconds-${hour}*3600)/60))
    local sec=$(($seconds-${hour}*3600-${min}*60))
    ((hour > 0)) && echo $hour小时/$min分/$sec秒 && return
    ((min > 0)) && echo $min分/$sec秒 && return
    echo $sec秒
}

function work_finish() {
    umount_workspace

    if [ "$1" != "" ]; then
        endTime=`date +%Y%m%d-%H:%M:%S`
        endTime_s=`date +%s`
        sumTime=$[ $endTime_s - $startTime_s ]

        if [ "$2" == "succeeded" ]; then color=32m; else color=31m; fi
        echo ""
        echo -e "\033[$color$1 $2 !   编译详情请查看: vim $BUILDING_WORKSPACE_PATH/out/log\033[0m"
        echo -e "\033[$color$startTime ---> $endTime" "    耗时：$(format_time $sumTime)\033[0m\n"
    fi
    exit
}

function exit_when_error() {
    if [ $2 != 0 ]; then
        work_finish $1 failed
    fi
}

function accent_echo() {
    echo ""
    echo -e $1
    echo ""
}

function make_appmanager() {
    sudo mkdir -p $BUILDING_WORKSPACE_PATH/out/appmanager
    sudo rm -rf $BUILDING_WORKSPACE_PATH/out/appmanager/*
    cd $BUILDING_WORKSPACE_PATH/out/appmanager
    echo "make appmanager"
    sudo /home/env/anaconda3/bin/pyinstaller $BUILDING_WORKSPACE_PATH/$CODE_DIR/tools/scripts/appmanager/installer.py &>> $BUILDING_WORKSPACE_PATH/out/log
    if [ $? != 0 ]; then
        cat << EOF_CAT
需要安装以下软件：
    sudo apt install python3-pip
    sudo pip3 install setuptools
    sudo pip3 install wheel
    sudo pip3 install pyinstaller
EOF_CAT
        work_finish
    fi

    sudo mkdir -p $DEB_PATH/appmanager/
    sudo cp -r $BUILDING_WORKSPACE_PATH/out/appmanager/dist/installer/* $DEB_PATH/appmanager/
    sudo mv $DEB_PATH/appmanager/installer $DEB_PATH/appmanager/wine-appmanager
    sudo chown $(logname):$(logname) -R $DEB_PATH/appmanager/
    sudo rm -rf /opt/winux/tools/appmanager/*
    sudo cp -r $DEB_PATH/appmanager/* /opt/winux/tools/appmanager/
    sudo rm -rf $BUILDING_WORKSPACE_PATH/$CODE_DIR/tools/scripts/appmanager/__pycache__
    echo ""
}

if [[ $EUID -ne 0 ]]; then
  accent_echo "\033[31m请加 sudo 执行该脚本!\033[0m"
  exit
fi

if [ -f $BUILDING_WORKSPACE_PATH/out/log ];then
    sudo rm $BUILDING_WORKSPACE_PATH/out/log
fi
parse_params $@
mount_workspace

if [ "$1" == "make" ]; then
    accent_echo "\033[32m已经屏蔽了编译的输出，可以在新窗口查看: tail -f $BUILDING_WORKSPACE_PATH/out/log\033[0m"
    for app in ${PARAM_APPS[@]}; do
        make_project $1 $CHROOT_I386 $app $BUILD_32_DIR_NAME
        if [ $ONLY32 == 0 ]; then
            make_project $1 $CHROOT_AMD64 $app $BUILD_64_DIR_NAME
            make_project $1 $CHROOT_I386 $app $BUILD_32_64_DIR_NAME
        fi
        echo ""
    done

elif [ "$1" == "remake" ]; then
    accent_echo "\033[32m已经屏蔽了编译的输出，可以在新窗口查看: tail -f $BUILDING_WORKSPACE_PATH/out/log\033[0m"
    # make_appmanager
    for app in ${PARAM_APPS[@]}; do
        remake_project $1 $CHROOT_I386 $app $BUILD_32_DIR_NAME
        if [ $ONLY32 == 0 ]; then
            remake_project $1 $CHROOT_AMD64 $app $BUILD_64_DIR_NAME --enable-win64
            remake_project $1 $CHROOT_I386 $app $BUILD_32_64_DIR_NAME --with-wine64=../$BUILD_64_DIR_NAME --with-wine-tools=../$BUILD_32_DIR_NAME
        fi
    done

elif [ "$1" == "install" ]; then
    accent_echo "\033[32mdo make install ......\033[0m"

elif [ "$1" == "deb" ]; then
    if [ -z "$DEB_VERSION" ]; then
        accent_echo "\033[31m火锅打包必须指定版本号！\033[0m"
        work_finish
    fi
    PARAM_INSTALL_DIR=no

elif [ "$1" == "appmanager" ]; then
    # make_appmanager
    work_finish

else
    usage

fi

if [ "$PARAM_INSTALL_DIR" != "no" ]; then
    install_project $1
fi

if [ ! -z "$DEB_VERSION" ]; then
    accent_echo "\033[32m开始火锅打包 hotpot6_"$DEB_VERSION"_amd64.deb ......\033[0m"
    mkdir -p $DEB_PATH
    cp $BUILDING_WORKSPACE_PATH/$CODE_DIR/tools/scripts/hotpot_maker/auto_yuanyanghotpot_deb.sh $DEB_PATH
    cp -r $BUILDING_WORKSPACE_PATH/$CODE_DIR/tools/scripts/hotpot_maker/DEBIAN $DEB_PATH
    # cp -r $BUILDING_WORKSPACE_PATH/$CODE_DIR/tools/scripts/hotpot_maker/uitools $DEB_PATH
    cd $DEB_PATH
    chmod +x auto_yuanyanghotpot_deb.sh
    rm -rf hotpot6_$DEB_VERSION*
    sudo ./auto_yuanyanghotpot_deb.sh $BUILDING_WORKSPACE_PATH/out/install $DEB_VERSION
fi

work_finish $1 succeeded
