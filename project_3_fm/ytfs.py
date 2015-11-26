from optparse import OptionParser
from os.path import isfile, isdir

BUCKET_NAME = 'ytfs-felipe-yaling-garrett'
DB_NAME = 'ytfs.db'
TABLE_NAME = 'mp3'

def run(root_dir,
        mount_dir,
        search_dir,
        download_flg, 
        upload_flg):
        
    errors = []
    file_or_dir = {}

    retval, error, conn = init_db()
    if conn is None:
        return False, error
    
    if download_flg is True:
        file_or_dir['action'] = 'download'
        file_or_dir['path'] = root_dir
    elif upload_flg is True:
        file_or_dir['action'] = 'upload'
        file_or_dir['path'] = root_dir
          
    if download_flg is True:
        download_files(file_or_dir)
        files_to_process = get_files_from_dir(file_or_dir['path']) 
    elif upload_flg is True:           
        files_to_process = []
        files_to_process = get_files_from_dir(file_or_dir['path'])         
        upload_files(files_to_process, file_or_dir)

    errors = parse_header_info(conn = conn, files = files_to_process)
        
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
        cursor.execute('CREATE UNIQUE INDEX ukey ON {table} '\
            '(file, title, artist, album, genre, year);'\
            .format(table = TABLE_NAME))
    except sqlite3.OperationalError as e:
        error = 'Error initializing DB. Error: {e}.'.format(e = e)

    return True, error, conn

def get_files_from_dir(file_or_dir):
    from os import walk
    from os.path import abspath, dirname
    
    files_to_process = []
    
    for (dirpath, dirnames, filenames) in walk(file_or_dir):
        for file_name in filenames:
            complete_path = abspath(dirpath) + '/'
            complete_path += file_name
            complete_path = complete_path.replace('//', '/')
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

    if title == 'Unknown':
        title = file[file.rindex('/') + 1:]
    else:
        title = title.encode('utf-8').replace(' ', '_') + '.mp3'
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
    try:
        cursor.execute(query)
        conn.commit()
    except sqlite3.IntegrityError:
        pass
    
if __name__ == '__main__':
    import os

    root_dir = os.getenv('YTFS_ROOT_DIR', None)
    mount_dir = os.getenv('YTFS_MOUNT_DIR', None)
    search_dir = os.getenv('YTFS_SEARCH_DIR', None)
    download_flg = os.getenv('YTFS_DOWNLOAD_FLG', None)
    upload_flg = os.getenv('YTFS_UPLOAD_FLG', None)

    root_dir = root_dir.strip() if root_dir is not None else root_dir
    mount_dir = mount_dir.strip() if mount_dir is not None else mount_dir
    search_dir = search_dir.strip() if search_dir is not None else search_dir
    download_flg = download_flg.strip().lower() if download_flg is not None else download_flg
    upload_flg = upload_flg.strip().lower() if upload_flg is not None else upload_flg
    
    params = [{'param': 'YTFS_ROOT_DIR', 'is_valid': root_dir is not None \
                and isdir(root_dir)},
              {'param': 'YTFS_MOUNT_DIR', 'is_valid': mount_dir is not None \
                and isdir(mount_dir)},
              {'param': 'YTFS_SEARCH_DIR', 'is_valid': search_dir is not None \
                and isdir(search_dir)},
              {'param': 'YTFS_DOWNLOAD_FLG', 'is_valid': download_flg is not None \
                and download_flg in ['true', 't', '1', 'false', 'f', '0']},
              {'param': 'YTFS_UPLOAD_FLG', 'is_valid': upload_flg is not None \
                and upload_flg in ['true', 't', '1', 'false', 'f', '0']}]

    for param in params:
        if not param['is_valid']:
            raise Exception('{param} is undefined/invalid. Please define {param} in '\
                'an .env file. Then type source <.env file> from the command '\
                'line before running this script.'.format(param = param['param']))

    if download_flg in ['true', 't', '1']:
        download_flg = True
    elif download_flg in ['false', 'f', '0']:
        download_flg = False

    if upload_flg in ['true', 't', '1']:
        upload_flg = True
    elif upload_flg in ['false', 'f', '0']:
        upload_flg = False

    if download_flg is True and upload_flg is True:
        raise Exception('Download/upload are mutually exclusive. Both cannot be true.')

    if mount_dir == root_dir:
        raise Exception('MOUNT_DIR and ROOT_DIR must be different.')
        
    retval, errors = run(
        root_dir = root_dir,
        mount_dir = mount_dir,
        search_dir = search_dir,
        download_flg = download_flg,
        upload_flg = upload_flg
    )
    
    if errors:
        import pprint
        pprint.pprint(errors)
        

