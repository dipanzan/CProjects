from optparse import OptionParser
from google_drive_downloader import GoogleDriveDownloader as gdd
parser = OptionParser()
parser.add_option(
    "-i",
    "--id" ,
    dest="id",
    help="google drive url id",
    default="1TpgZlmm2ByhsTNTrY4WHGwYWPZHYgt10"
)
parser.add_option(
    "-p",
    "--path",
    dest="path",
    help="download full path",
    default="/home/amkoyan/h2r-tmp/src/Data"
)

(options, args) = parser.parse_args()

dest_file = options.path + '/dataSrc.tar.gz' 
gdd.download_file_from_google_drive(file_id=options.id,
        dest_path=dest_file,
        unzip=False)