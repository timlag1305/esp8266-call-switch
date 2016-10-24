import argparse, glob, os, shutil

parser = argparse.ArgumentParser()
parser.add_argument('output_format', help='choose the directory structure format [arduino|platformio]')
args = parser.parse_args()
src_dir = 'src'

if args.output_format == 'arduino':
    in_file = '*.cpp'
    out_file = '*.ino'

    for filename in glob.iglob(os.path.join(src_dir, '*.cpp')):
        os.rename(filename, filename[:-4] + '.ino')

        if not os.path.exists(filename[:-4]):
            os.makedirs(filename[:-4])

        shutil.move(filename[:-4] + '.ino', filename[:-4])
elif args.output_format == 'platformio':
    in_file = '*.ino'
    out_file = '*.cpp'

    for filename in glob.iglob(os.path.join(src_dir, '**/*.ino'), recursive=True):
        os.rename(filename, filename[:-4] + '.cpp')
        newfilename = filename[:-4] + '.cpp'
        dir_struct = newfilename.split('/')
        dir_struct.pop(-2)
        dest = '/'.join(dir_struct)
        shutil.move(newfilename, dest)
        os.rmdir(filename[:filename.rfind('/')])
