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
        
    retval, error, conn = init_db()
    if conn is None:
        return False, error
    
    local_files = get_files_from_root_dir(root_dir) 
    
    if download_flg is True:
        downloaded_files = download_files(files = local_files,
                                          download_folder = root_dir)
        if downloaded_files:
            local_files.extend(downloaded_files)
    elif upload_flg is True:       
        upload_files(files = local_files, upload_folder = root_dir)
    
    errors = update_database(conn = conn, files = local_files)
        
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

def get_files_from_root_dir(root_dir):
    from os import walk
    from os.path import abspath, dirname
    
    files_to_process = []
    
    for (dirpath, dirnames, filenames) in walk(root_dir):
        for file_name in filenames:
            complete_path = abspath(dirpath) + '/'
            complete_path += file_name
            complete_path = complete_path.replace('//', '/')
            files_to_process.append({'name': file_name, 'path': complete_path})
        
    return sorted(files_to_process)
    
def download_files(files, download_folder):                    
    import boto
    s3 = boto.connect_s3()
    filenames = [file['name'] for file in files]
    newly_downloaded_files = []
    
    print('Downloading files to directory = {dir}.'\
        .format(dir = download_folder))
        
    bucket = s3.get_bucket(BUCKET_NAME)  
    for key in bucket.list():
        # If file is already downloaded, skip
        if key:
            if key.name in filenames:
                continue
            if key.name.endswith('.mp3'):
                complete_path = download_folder + '/' + key.name
                complete_path = complete_path.replace('//', '/')
                key.get_contents_to_filename(complete_path)
                newly_downloaded_files.append(
                    {'name': key.name, 
                     'path': complete_path}
                 )
    return newly_downloaded_files
    
def upload_files(files, upload_folder):                    
    import boto
    s3 = boto.connect_s3()
    num_skipped = 0
    
    # Only upload files that are not already in the remote bucket
    bucket = s3.get_bucket(BUCKET_NAME)  
    for key in bucket.list():
        if key and key.name:
            for i in range(len(files)):
                file = files[i]
                if file['name'] == key.name:
                    del files[i]
                    num_skipped += 1
                    break
    
    print('Uploading files {files} in {dir}. Number skipped: {skipped}.'\
            .format(files = str(files),
                    dir = upload_folder,
                    skipped = num_skipped))
    
    for file in files:
        file_name = file[file.rindex('/') + 1:]
        if file_name.endswith('.mp3'):
            bucket = s3.create_bucket(BUCKET_NAME)  
            key = bucket.new_key(file_name)
            key.set_contents_from_filename(file)
            key.set_acl('public-read')

def update_database(conn, files):
    import eyed3
    from eyed3.mp3 import isMp3File
    
    errors = []

    for file in files:
        if not isMp3File(file['path']):
            errors.append('{fi} is not an mp3 file.'.format(fi = file['name']))
            continue
        mp3_file = eyed3.load(file['path'])
        tag = mp3_file.tag
        if tag:
            artist = mp3_file.tag.artist
            album = mp3_file.tag.album
            title = mp3_file.tag.title
            genre = mp3_file.tag.genre
            year = mp3_file.tag.release_date or mp3_file.tag.recording_date
            if genre:
                genre = genre.name
            insert_into_db(conn = conn, file = file, artist = artist, 
                album = album, title = title, genre = genre, year = year)
        else:
            errors.append('No tag found for mp3 file {fi}.'.format(fi = file['name']))
            # All values default to 'Unknown'
            insert_into_db(conn = conn, file = file) 
    if errors:
        return False, errors

    return True, ''

def insert_into_db(conn, file, title = 'Unknown', artist = 'Unknown', 
    album = 'Unknown', genre = 'Unknown', year = 'Unknown'):
    import sqlite3
    conn = sqlite3.connect(DB_NAME)
    
    if title == 'Unknown':
        title, album, artist, genre, year = search_apple_api(file['name'])

    if title == 'Unknown':
        title = file['name']
    else:
        title = title.encode('utf-8') + '.mp3'
    artist = artist.encode('utf-8')
    album = album.encode('utf-8')
    genre = genre.encode('utf-8')
    year = str(year).encode('utf-8')

    cursor = conn.cursor()
    query = 'INSERT INTO {table} (file, title, artist, '\
        'album, genre, year) VALUES("{file}", "{title}", "{artist}", '\
        '"{album}", "{genre}", "{year}");'\
        .format(table = TABLE_NAME, file = file['path'], title = title, 
                artist = artist, album = album, genre = genre, 
                year = year)
    try:
        cursor.execute(query)
        conn.commit()
    except sqlite3.IntegrityError:
        pass
        
def search_apple_api(filename):
    import requests
    import re
    from datetime import datetime
    
    album = None
    artist = None
    genre = None
    title = None
    year = None
    
    base_url = 'https://itunes.apple.com/search?term='
    filename = filename.replace('.mp3', '').replace('_', '+')
    query = re.sub('[^A-Za-z0-9+]+', '', filename)

    req = requests.get(base_url + query + '&limit=1')
    if req and req.status_code == 200:
        json = req.json()
        if json['results']:
            try:
                result = json['results'][0]
                title = result['trackName']
                album = result['collectionName'] 
                artist = result['artistName']
                genre = result['primaryGenreName']
                year = result['releaseDate']
                if year:
                    year = year[:year.index('-')]
                    if not isnumeric(year):
                        year = None
            except:
                pass
    
    album = album if album else 'Unknown'
    artist = artist if artist else 'Unknown'
    genre = genre if genre else 'Unknown'
    title = title if title else 'Unknown'
    year = year if year else 'Unknown'
    
    return title, album, artist, genre, year
    
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
        

