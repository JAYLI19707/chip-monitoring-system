import gzip
import shutil

with gzip.open('ramdisk.gz', 'rb') as f_in:
    with open('ramdisk.gz', 'wb') as f_out:
        shutil.copyfileobj(f_in, f_out)
