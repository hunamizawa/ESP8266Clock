#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
data_dir ディレクトリ内のすべてのファイルを、PGM_P ( = const char * ) として埋め込むスクリプト。
Build/Upload の前に、自動的に実行される。
"""

Import("env", "projenv")
import os, glob, re, hashlib

header_file_header = """// auto-generated by script tools/embed_resource.py
// DO NOT EDIT BY HAND

#ifndef ESP8266Clock_resource_data_H_
#define ESP8266Clock_resource_data_H_

#include <Arduino.h>

namespace Resource {
"""

header_file_footer = """
} // namespace Resource

#endif // ESP8266Clock_resource_data_H_
"""

cpp_file_header = """// auto-generated by script tools/embed_resource.py
// DO NOT EDIT BY HAND

#include <resource.h>
#include <resource-data.h>

namespace Resource {
"""

cpp_file_footer = """
} // namespace Resource
"""

data_dir = env['PROJECT_DATA_DIR']
src_dir = env['PROJECT_SRC_DIR']

# 変数名に使えない文字を削除する用
pattern = re.compile(r'[^0-9A-Za-z_]')


def del_invalid_char(input):
  """変数名に使えない文字をアンダーバーに置換"""
  return pattern.sub('_', input)


def get_var_name(file_path):
  """ファイルパスから変数名を取得する"""
  # 数字から始まるファイル名対策として、先頭のパス区切り文字をわざと残す
  return del_invalid_char(file.replace(data_dir, ''))


def embed_as_binary(file_path, header_file, cpp_file):
  """ファイル file_path を .h/.cpp ファイルに埋め込む"""
  var_name = get_var_name(file_path)

  with open(file_path, 'rb') as f:
    bs = f.read()

  header_file.write('extern const char   %s[] PROGMEM;\n' % (var_name))
  header_file.write('extern const size_t len%s;\n' % (var_name))
  header_file.write('extern const char   etag%s[] PROGMEM;\n' % (var_name))

  cpp_file.write('const char %s[] PROGMEM = {\n' % (var_name))
  for i, b in enumerate(bs):
    cpp_file.write('%s,' % (hex(b)))
    if i % 32 == 31:
      cpp_file.write('\n')
  cpp_file.write('};\n')
  cpp_file.write('const size_t len%s = %d;\n' % (var_name, len(bs)))
  hs = hashlib.sha256(bs).hexdigest()
  cpp_file.write('const char etag%s[] PROGMEM = "W/\\"%s\\"";\n' % (var_name, hs[:12]))


with open(os.path.join(src_dir, 'resource-data.h'), 'w') as header_file:
  with open(os.path.join(src_dir, 'resource.cpp'), 'w') as cpp_file:

    header_file.write(header_file_header)
    cpp_file.write(cpp_file_header)

    files = glob.glob(os.path.join(data_dir, '**'))
    for file in files:
      embed_as_binary(file, header_file, cpp_file)
      print("Embedding %s" % (file))

    cpp_file.write('resource_t searchByPath(const String &path) {\n')
    for file in files:
      cpp_file.write('  if (path == F("%s"))\n' % (file.replace(data_dir, '').replace('\\', '/')))
      var_name = get_var_name(file)
      cpp_file.write('    return {%s, len%s, etag%s};\n' % (var_name, var_name, var_name))
    cpp_file.write('  return {0, 0, 0};\n')
    cpp_file.write('}')

    header_file.write(header_file_footer)
    cpp_file.write(cpp_file_footer)