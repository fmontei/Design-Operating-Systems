=== SETUP ===
1) install sqlite3 for C: http://www.tutorialspoint.com/sqlite/sqlite_installation.htm
2) install pip: http://pip.readthedocs.org/en/stable/installing/
3a) run: sudo pip install -U boto
3b) run: sudo pip install -U eyed3
4) Create file boto.cfg from the credentials.csv file I'm attaching. 

Include the [Credentials] header. 
Fill in the pieces in {} with the data in credentials.csv.
credentials.csv file is not on Github for security reasons.

boto.cfg format:

[Credentials]
aws_access_key_id = {ACCESS KEY ID}
aws_secret_access_key = {SECRET ACCESS KEY}

4) Put the file in /etc/boto.cfg

=== USAGE ===
1) Download: python ytfs.py -u {file or folder you want to upload}
2) Upload: python ytfs.py -d {ABSOLUTE PATH to folder you want to download
   all files stored in the cloud to}
   
   Example of absolute path: /home/osboxes/Desktop/Design-Operating-Systems/project_3_fm

=== DEPENDENCIES ==
-pip
-boto (Python module for storing music in cloud) 
-eyed3 (Python module for extracting mp3 headers)


