#!/bin/bash
#######################################################
# $Version:      v1.0
# $Function:     Shell Template Script
# $Author:       wml
# $Create Date:  2018-04-26 09:30
# $Description:  copy source code uitools/core/ to system
#pip install pyinstaller
#######################################################

src_path=`pwd`		#uitools/core/
dest_path=/home/dev/uiins
#copyed_src_path="$dest_path"/src
#pyed_exec_path="$dest_path"/pyexec
current_user=`id -un 1000`

# Shell Usage
shell_usage(){
    echo $"Usage: $0 {install_all/install_common/uninstall}"
}

#Write Log 
shell_log(){
    LOG_INFO=$1
    echo "$(date "+%Y-%m-%d") $(date "+%H-%M-%S") : $0 : ${LOG_INFO}" 
}


pipelight_cfg(){
	  # update 50iscas
	  sed -i "s/@iscas@/$SUDO_USER/g" /etc/udev/rules.d/50-iscas.rules  

	  #dev
	  sudo udevadm control --reload

	  # pipelight for ukey
	  sudo pipelight-plugin --unlock nphdzb
	  sudo pipelight-plugin --enable nphdzb
	  sudo pipelight-plugin --unlock npccbnetsign
	  sudo pipelight-plugin --enable npccbnetsign
	  sudo pipelight-plugin --unlock npactivex
	  sudo pipelight-plugin --enable npactivex

	  pipconflist=`find /usr/share/pipelight/configs/ -type f -name "pipelight-*"`;

	  for conf in $pipconflist;
	  do
	    echo $conf;
	    #sed -i -E 's/\$share\/wine(64){0,1}/$HOME\/APPS\/ccb/g' $conf;
	    sed -i -E 's/\$HOME\/\.wine(-pipelight){0,1}(64){0,1}/$HOME\/APPS\/ccb/g' $conf;
	  done;
}


function install_common_files()
{
        echo "install_common_files"
	cd $src_path

	cp etc/* /etc/ -R
        pipelight_cfg

	cp usr/* /usr/ -R
        gtk-update-icon-cache -f /usr/share/icons/hicolor/
}


function copy_src_file()
{
	echo "copy_src_file"
        if [[ ! -d "$dest_path/src" ]]; then
		#statements
		mkdir "$dest_path/src" -p
        fi
	rm "$dest_path/src"/* -R
	#mkdir "$dest_path"/src 
	cp "$src_path"/opt/winux/tools/appmanager "$dest_path"/src/ -R

	chown "$current_user":"$current_user" "$dest_path"/* -R

}

function mk_dest_path()
{
	if [[ ! -d "$dest_path/pyexec" ]]; then
		#statements
		mkdir "$dest_path/pyexec" -p
		chown $current_user:$current_user "$dest_path"/pyexec -R
	fi
}

function clean_dest_path()
{
	if [[ -d "$dest_path/pyexec" ]]; then
		#statements
		rm "$dest_path/pyexec" -r
	fi
}

# python2exec src_path src_file 
function python2exec()
{
	if [[ $# != 2 ]]; then
		#statements
		echo "usage: python2exec target_path target_file "
		return
	fi

	target_path=$1
	target_file=$2 

	cd "$dest_path"/pyexec
	#pyinstaller "$target_path"/"$target_file"
	shell_log "$target_path/$target_file"
	sudo -u $current_user -H bash -c "pyinstaller $target_path/$target_file"

}

function cp_python2exe_dist_files()
{
	target=$1
	#cp "$dest_path"/pyexec/dist/"$target"/* "$target_path" -R
	if [ ! -x "$2" ]; then
		shell_log "mkdir $2"
		mkdir -p "$2";
	fi
	cp "$dest_path"/pyexec/dist/"$target"/* "$2" -R;
	chown $current_user:$current_user -R $2;

}

function install_tools_appmanager()
{
	#installer
	mk_dest_path
	shell_log "pyinstaller appmanager-installer"
	python2exec "$dest_path"/src/appmanager installer.py
        if [[ ! -d /opt/winux/tools/appmanager ]]; then
		#statements
		mkdir /opt/winux/tools/appmanager -p
        fi
        rm  -rf /opt/winux/tools/appmanager/*
	cp_python2exe_dist_files installer /opt/winux/tools/appmanager
	mv /opt/winux/tools/appmanager/installer /opt/winux/tools/appmanager/wine-appmanager
	cp -a "$dest_path/src/appmanager/hdpi" /opt/winux/tools/appmanager/
	cp -a "$dest_path/src/appmanager/status" /opt/winux/tools/appmanager/
	clean_dest_path

	chown $current_user:$current_user -R /opt/winux/tools/appmanager

}

function install_python_files()
{

	install_tools_appmanager

}


# Main Function
main(){
        
    echo "start"

    case $1 in
        all)
            echo "install_python"
            copy_src_file
            install_python_files
            install_common_files
            ;;
        common)
            echo "install_common"
	    install_common_files
	    ;;
        python)
            echo "install_python"
	    copy_src_file
            install_python_files
            ;;
        *)
            shell_usage;
    esac
        
    echo "finish"
}

#Exec
main $1


