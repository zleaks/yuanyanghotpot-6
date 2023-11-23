#!/usr/bin/python3

import sys,time,os,stat
from shutil import copyfile

def main(argv):
   if len(argv) < 1:
        sys.stderr.write("invalid params!!")
        exit(1)
   updated_dir=argv[0]
   all_config_dirs={}
   if not os.path.exists(updated_dir):
      os.mkdir(updated_dir)
   updated_timestamp=int(round(time.time()*1000))
   with open(updated_dir+'/.updatetime', 'w') as f_update:
      f_update.write(str(updated_timestamp));
      f_update.close()
   os.system("chmod 777 '%s'/.updatetime"%updated_dir)
   all_config_dirs={updated_dir:updated_timestamp}

   filename = os.environ['HOME']+'/.local/share/wine-xdg/configs-index'
   if not os.path.exists(filename):
      #os.system("mkdir {}".format(os.environ['HOME']+'/.local/share/wine-xdg'))
      os.system("touch {}".format(filename))
      os.chmod(filename,stat.S_IRWXU | stat.S_IRGRP | stat.S_IXGRP | stat.S_IROTH | stat.S_IXOTH)
   with open(filename, 'r+') as f_index:
      for line in f_index:
         configdir=line.strip('\n')
         if not os.path.exists(configdir+'/.updatetime'):
            f_index.write('')
            continue
         with open(configdir+'/.updatetime', 'r') as fconfigupdate:
            configdirupdate=int(fconfigupdate.read(13))
            fconfigupdate.close()
         all_config_dirs[configdir]=configdirupdate
      f_index.close()
   sorted_all_config_dirs = sorted(all_config_dirs.items(), key=lambda x: x[1])
   print(sorted_all_config_dirs)

   avail_paths={}
   arr_dirs=[elem[0] for elem in sorted_all_config_dirs]
   for arr_path in arr_dirs:
      file_list = os.listdir(arr_path)
      for file_name in [elem for elem in file_list if elem.endswith('.desktop')]:
         path = os.path.join(arr_path, file_name)
         avail_paths[file_name]=path
   print (avail_paths)

   destpath_root=os.environ['HOME']+'/.local/share/applications'
   if not os.path.exists(destpath_root):
      os.system("mkdir {}".format(destpath_root))
   home_file_list = os.listdir(destpath_root)
   for home_file_name in [elem for elem in home_file_list if elem.endswith('.desktop')]:
      home_path = os.path.join(destpath_root, home_file_name)
      os.remove(home_path)
   for key in avail_paths:
      destpath=destpath_root+'/'+key
      print('copy xdg file from '+avail_paths[key]+' to '+destpath)
      copyfile(avail_paths[key],destpath)

   os.system('update-desktop-database '+os.environ['HOME']+'/.local/share/applications')

   with open(filename, 'w') as findex:
      findex.write('\n'.join(arr_dirs))
      findex.close()

if __name__ == "__main__":
   main(sys.argv[1:])
