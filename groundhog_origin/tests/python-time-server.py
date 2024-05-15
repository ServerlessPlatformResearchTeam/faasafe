import logging
import requests
from ctypes import *
so_file = "../build/groundhog-tracee-lib.so"
groundhog_functions = CDLL(so_file)

logging.basicConfig(level=logging.DEBUG)
s = requests.Session()
s.get('http://httpbin.org/cookies/set/sessioncookie/123456789')
groundhog_functions.checkpoint_me()
s.get('http://httpbin.org/cookies/set/anothercookie/123456789')
r = s.get("http://httpbin.org/cookies")
groundhog_functions.restore_me()
print(r.text)
