#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
ビルド完了後、バイナリを圧縮＆署名する。
"""

Import("env", "projenv")
import os, io, sys, subprocess, struct, shutil

config = env.GetProjectConfig()

def get_config_or_default(section, option, default):
  """platformio.ini から設定を探し、なければデフォルト値を使う"""
  try:
    return config.get(section, option)
  except:
    return default

pubkey_path = os.path.join(env.subst("$PROJECT_DATA_DIR"), 'public.key')
if not os.path.isfile(pubkey_path):
  sys.stderr.write("Error: public.key not found. Locate it to data_dir.\n")
  env.Exit(1)

def sign_file(src, dest, privkey_path):
  """ファイルに署名する"""
  openssl_path = get_config_or_default('env:bin_release', 'openssl_path', 'openssl')
  with open(src, 'rb') as f_in:
    signcmd = [openssl_path, 'dgst', '-sha256', '-sign', privkey_path]
    proc = subprocess.run(signcmd, stdin=f_in, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, check=True)
    f_in.seek(0)

    with open(dest, 'wb') as f_out:
      shutil.copyfileobj(f_in, f_out)
      f_out.write(proc.stdout)
      f_out.write(struct.pack('<L', len(proc.stdout)))

def signing_binary(source, target, env):

  bin_base = os.path.join(env.subst("$BUILD_DIR"), env.subst("$PROGNAME"))
  bin_path = bin_base + '.bin'
  out_path = bin_base + '.signed.bin'
  privkey_path = config.get('env:bin_release', 'private_key', os.path.join(env.subst("$PROJECT_DIR"), 'private.key'))

  if not os.path.isfile(privkey_path):
    sys.stderr.write("Error: private.key not found. Locate it to project root directory.\n")
    env.Exit(1)

  sign_file(bin_path, out_path, privkey_path)

env.AddPostAction(
  #"checkprogsize",
  "$BUILD_DIR/${PROGNAME}.bin",
  env.VerboseAction(
    signing_binary,
    "Signing $BUILD_DIR/${PROGNAME}.signed.bin"))
