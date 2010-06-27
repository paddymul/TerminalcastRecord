
import os


file_names = [
    "charproc.c",
    "ptydata.c",
    "main.c",
    "menu.c"]

symbols = [
    "APOLLO_SR9",
    "apollo",
    "AIXV3",
    "WIN32",
    "VMS"]

for file_name in file_names:
    temp_file_name = file_name + "tmp"
    for symbol in symbols:
        os.system("unifdef  -U%s %s > %s " % (symbol, file_name, temp_file_name))
        os.system("mv %s %s " % (temp_file_name, file_name))
