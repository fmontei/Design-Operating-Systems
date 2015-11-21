from optparse import OptionParser
from os.path import isfile, isdir

BUCKET_NAME = 'ytfs-felipe-yaling-garrett'

def run(download = None, 
        upload = None):
        
    file_or_dir = {}
    
    if download:
        file_or_dir['action'] = 'download'
        file_or_dir['path'] = download
        if not isdir(download):
            return False, 'Download folder is invalid.'
    elif upload:
        file_or_dir['action'] = 'upload'
        file_or_dir['path'] = upload
        if isfile(upload):
            file_or_dir['type'] = 'file'
        elif isdir(upload):
            file_or_dir['type'] = 'dir'
        else:
            return False, 'Upload file/folder is invalid.'
          
    if download:
        download_files(file_or_dir)
    elif upload:           
        files_to_process = []
        if file_or_dir['type'] == 'dir':
            files_to_process = get_files_from_dir(file_or_dir['path'], 
                is_dir = True) 
        elif file_or_dir['type'] == 'file':
            files_to_process = get_files_from_dir(file_or_dir['path'],
                is_dir = False)          
        upload_files(files_to_process, file_or_dir)
        
    return True, ''

def get_files_from_dir(file_or_dir, is_dir = False):
    from os import walk
    from os.path import dirname, abspath
    
    files_to_process = []
    
    if is_dir:
        for (dirpath, dirnames, filenames) in walk(file_or_dir):
            for file_name in filenames:
                complete_path = dirname(abspath(file_name)) + '/'
                complete_path += dirpath + '/'
                complete_path += file_name
                complete_path = complete_path.replace('//', '/')
                files_to_process.append(complete_path)
    else:
        complete_path = dirname(abspath(file_or_dir)) + '/'
        complete_path += file_or_dir
        files_to_process.append(complete_path)
        
    return sorted(files_to_process)
    
def download_files(file_or_dir):                    
    import boto
    s3 = boto.connect_s3()
    
    print('Downloading files.')
    
    download_folder = file_or_dir['path']
    
    bucket = s3.get_bucket(BUCKET_NAME)  
    for key in bucket.list():
        complete_path = download_folder + '/' + key.name
        complete_path = complete_path.replace('//', '/')
        key.get_contents_to_filename(complete_path)
    
def upload_files(files, file_or_dir):                    
    import boto
    s3 = boto.connect_s3()
    
    print('Uploading files {files} in {dir}.'\
            .format(files = str(files),
                    dir = 'current directory' if file_or_dir['type'] == 'file'\
                        else file_or_dir['path']))
    
    for file in files:
        file_name = file[file.rindex('/') + 1:]
        bucket = s3.create_bucket(BUCKET_NAME)  
        key = bucket.new_key(file_name)
        key.set_contents_from_filename(file)
        key.set_acl('public-read')
    
if __name__ == '__main__':
    parser = OptionParser()
    parser.add_option("-u", "--upload", dest="upload",
                      help="File to upload or folder containing files to "\
                        "upload.")
    parser.add_option("-d", "--download", 
                      help = "File to upload or folder containing files to "\
                        "download.")

    (options, args) = parser.parse_args()
    if options.download and options.upload:
        print('Download/upload are mutually exclusive.')
        
    retval, msg = run(
        download = options.download.strip() if options.download else None,
        upload = options.upload.strip() if options.upload else None
    )
    
    print retval, msg
        

