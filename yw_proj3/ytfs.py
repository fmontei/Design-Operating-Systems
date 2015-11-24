from optparse import OptionParser
from os.path import isfile, isdir

BUCKET_NAME = 'ytfs-felipe-yaling-garrett'
DB_NAME = 'ytfs.db'
TABLE_NAME = 'mp3'

def run(download = None, 
        upload = None):
        
    errors = []
    file_or_dir = {}

    retval, error, conn = init_db()
    if conn is None:
        return False, error
    
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
        files_to_process = get_files_from_dir(file_or_dir['path'], 
            is_dir = True) 
        errors = parse_header_info(conn = conn, files = files_to_process)
    elif upload:           
        files_to_process = []
        if file_or_dir['type'] == 'dir':
            files_to_process = get_files_from_dir(file_or_dir['path'], 
                is_dir = True) 
        elif file_or_dir['type'] == 'file':
            files_to_process = get_files_from_dir(file_or_dir['path'],
                is_dir = False)          
        upload_files(files_to_process, file_or_dir)
        
    return True, errors

def init_db():
    import sqlite3
    conn = sqlite3.connect(DB_NAME)
    cursor = conn.cursor()
    error = '' 

    try:
        cursor.execute('PRAGMA encoding="UTF-8";')
        cursor.execute('CREATE TABLE {table} '\
            '(id integer primary key autoincrement, file text, title text, '\
            'artist text, album text, genre text, '\
            'year text);'.format(table = TABLE_NAME))
        """
        cursor.execute('CREATE UNIQUE INDEX ukey ON {table} '\
            '(file, title, artist, album, genre, year);'\
            .format(table = TABLE_NAME))
        """
    except sqlite3.OperationalError as e:
        error = 'Error initializing DB. Error: {e}.'.format(e = e)

    return True, error, conn

def get_files_from_dir(file_or_dir, is_dir = False):
    from os import walk
    from os.path import abspath, dirname
    
    files_to_process = []
    
    if is_dir:
        for (dirpath, dirnames, filenames) in walk(file_or_dir):
            for file_name in filenames:
                complete_path = abspath(dirpath) + '/'
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
        if key.name.endswith('.mp3'):
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
        if file_name.endswith('.mp3'):
            bucket = s3.create_bucket(BUCKET_NAME)  
            key = bucket.new_key(file_name)
            key.set_contents_from_filename(file)
            key.set_acl('public-read')

def parse_header_info(conn, files):
    import eyed3
    from eyed3.mp3 import isMp3File
    
    errors = []

    for file in files:
        mp3_file = eyed3.load(file)
        if not isMp3File(file):
            errors.append('{file} is not an mp3 file.'.format(file = file))
        tag = mp3_file.tag
        if tag:
            artist = mp3_file.tag.artist
            album = mp3_file.tag.album
            title = mp3_file.tag.title
            genre = mp3_file.tag.genre
            year = mp3_file.tag.release_date or mp3_file.tag.recording_date
            if genre:
                genre = genre.name
            insert_into_db(conn = conn, file = file, artist = artist, album = album, 
                title = title, genre = genre, year = year)
        else:
            errors.append('No tag found for mp3 file {file}.'.format(file = file))
            insert_into_db(conn = conn, file = file) # All values default to 'Unknown'
    if errors:
        return False, errors

    return True, ''

def insert_into_db(conn, file, title = 'Unknown', artist = 'Unknown', 
    album = 'Unknown', genre = 'Unknown', year = 'Unknown'):
    import sqlite3
    conn = sqlite3.connect(DB_NAME)

    title = title.encode('utf-8').replace(' ', '_')
    artist = artist.encode('utf-8').replace(' ', '_')
    album = album.encode('utf-8').replace(' ', '_')
    genre = genre.encode('utf-8').replace(' ', '_')
    year = str(year).encode('utf-8').replace(' ', '_')

    cursor = conn.cursor()
    query = 'INSERT INTO {table} (file, title, artist, '\
        'album, genre, year) VALUES("{file}", "{title}", "{artist}", '\
        '"{album}", "{genre}", "{year}");'\
        .format(table = TABLE_NAME, file = file, title = title, 
                artist = artist, album = album, genre = genre, 
                year = year)
    cursor.execute(query)
    conn.commit()
    
if __name__ == '__main__':
    parser = OptionParser()
    parser.add_option("-u", "--upload", dest="upload",
                      help="File to upload or folder containing files to "\
                        "upload.")
    parser.add_option("-d", "--download", 
                      help = "Absolute path to download folder.")

    (options, args) = parser.parse_args()
    if options.download and options.upload:
        print('Download/upload are mutually exclusive.')
        
    retval, errors = run(
        download = options.download.strip() if options.download else None,
        upload = options.upload.strip() if options.upload else None
    )
    
    if errors:
        import pprint
        pprint.pprint(errors)
        

