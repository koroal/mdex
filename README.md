# mdex
CLI MangaDex downloader, written in C.
## Installation
- Make sure a C compiler, make, libcurl and libminizip are installed
- git clone https://github.com/koroal/mdex.git
- cd mdex
- make
- Move the generated 'mdex' file into your PATH
## Features
- Choose download language
- Choose chapters to download
- Resume interrupted downloads
- Detect multiple available chapter versions
- Choose chapter version based on the list of preferred scanlation groups
- Option to overwrite already downloaded files
- Option to override series title
- Option to include chapter title into the filename
- Option to check what will be done without actually downloading
- Option to download into a subdirectory instead of the current directory
## Usage
    mdex [-wsdnto] [-l lang] [-c list] series [group...]

    The first argument w/o dash must be a series link or uuid

    -w      Overwrite existing files
    -s      Save into a subdirectory
    -d      Report duplicate chapters
    -n      Only check and do not download
    -t      Include chapter title in filename
    -o      Override series title
    -l lang Choose language by code (default is 'en')
    -c list Choose chapters by ranges list (default is '-')

    The rest are scanlation groups in the order of preference
## Example
Reporting a chapter with multiple available translations (-d):

    $ mdex -sdn b905f827-8d48-4948-b58c-0d6fd330d10d -l en -c -3,20,21,30-32,63-
    BLAME! v01 c001: [Omanga], [Habanero Scans]
Preferred scanlation group chosen, reporting what will be done (-n):

    $ mdex -sdn b905f827-8d48-4948-b58c-0d6fd330d10d -l en -c -3,20,21,30-32,63- 'Omanga'
    Download: BLAME!/BLAME! v01 c001 [Omanga].cbz
    Download: BLAME!/BLAME! v01 c002 [Omanga].cbz
    Download: BLAME!/BLAME! v01 c003 [Omanga].cbz
    Download: BLAME!/BLAME! v04 c020 [Omanga].cbz
    Download: BLAME!/BLAME! v04 c021 [Omanga].cbz
    Download: BLAME!/BLAME! v05 c030 [Omanga].cbz
    Download: BLAME!/BLAME! v06 c031 [Omanga].cbz
    Download: BLAME!/BLAME! v06 c032 [Omanga].cbz
    Download: BLAME!/BLAME! v10 c063 [Omanga].cbz
    Download: BLAME!/BLAME! v10 c064 [Omanga].cbz
    Download: BLAME!/BLAME! v10 c065 [Omanga].cbz
The same command without -n will start downloading selected chapters:

    $ mdex -sd b905f827-8d48-4948-b58c-0d6fd330d10d -l en -c -3,20,21,30-32,63- 'Omanga'
    BLAME!/BLAME! v01 c001 [Omanga].cbz: 1/37
