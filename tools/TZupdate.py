#!/usr/bin/env python3
# -*- coding: utf-8 -*-

'''
このツールは、tzdata を利用して
1. 設定画面 HTML のために、地域名/都市名の選択肢を js として生成する。(../data/TZ.js)
2. 二分探索法によるルックアップテーブルをコード生成して、(../src/TZDB.h)
   a. 設定画面 HTML から POST されてきた地域名/都市名を検証する。
   b. 地域名/都市名の組から、<TZ.h> にある timezone 文字列を辞書引きする。

2. について
std::unordered_map を使いたいところだが、PROGMEM できない……
やむなく、コードの形でテーブルを保持する。
あと、strcmp() は 負, 0, 正 の3値を返すので二分探索にはもってこい。
参考にしたやつ(C#だけど): https://github.com/ufcpp/GraphemeSplitter/blob/master/GraphemeBreakPropertyCodeGenerator/Program.cs
'''

import io
import csv
import urllib.request
from typing import List

with urllib.request.urlopen('https://raw.githubusercontent.com/nayarsystems/posix_tz_db/master/zones.csv') as response:
  rawdata = response.read().decode('utf-8')

with io.StringIO(rawdata) as f:
  zones = csv.reader(f, delimiter=',', quotechar='"')

  cities = {}

  for row in zones:
    area, city = row[0].split('/', 1)
    if area not in cities:
      cities[area] = [city]
    else:
      cities[area].append(city)

# cities = {'Africa':[...], 'America':[...], ... }

area_maxlength = 0
city_maxlength = 0

with open('../data/TZ.js', 'w') as f:
  f.write('// auto-generated by script tools/TZupdate.py\n')
  f.write('var tz_cities = {')
  for area in cities:
    area_maxlength = max(area_maxlength, len(area))
    f.write('"%s":[' % (area))
    for city in cities[area]:
      city_maxlength = max(city_maxlength, len(city))
      f.write('"%s",' % (city))
    f.write('],')
  f.write('};')

with open('../src/TZDB.cpp', 'w') as f:
  f.write('''// auto-generated by script tools/TZupdate.py
// DO NOT EDIT BY HAND
//
// This database is autogenerated from IANA timezone database
//      https://www.iana.org/time-zones
// via: https://raw.githubusercontent.com/nayarsystems/posix_tz_db/master/zones.csv

#include "TZDB.h"

PGM_P TZDB::getTZ(const String &area, const String &city) {
  const char *a = area.c_str();
  const char *c = city.c_str();
  int r;
''' )

  def get_defname(area: str, city: str) -> str:
    return 'TZ_%s_%s' % (area, city.replace('/', '_').replace('-', 'm').replace('+', 'p'))

  def fc(sorted: List[str], lower: int, upper: int, indent: int) -> None:
    ws = ' ' * (indent * 2)

    if lower == upper:
      city = sorted[lower]
      defname = get_defname(area, city)
      f.write('%sif (strcmp_P(c, PSTR("%s")) == 0) return %s;\n' % (ws, city, defname))
      f.write('%selse return nullptr;\n' % (ws))

    elif lower + 1 == upper:
      city = sorted[lower]
      defname = get_defname(area, city)
      f.write('%sif (strcmp_P(c, PSTR("%s")) == 0) return %s;\n' % (ws, city, defname))

      city = sorted[upper]
      defname = get_defname(area, city)
      f.write('%selse if (strcmp_P(c, PSTR("%s")) == 0) return %s;\n' % (ws, city, defname))
      f.write('%selse return nullptr;\n' % (ws))

    else:
      middle = lower + (upper - lower) // 2
      city = sorted[middle]
      defname = get_defname(area, city)

      f.write('%sr = strcmp_P(c, PSTR("%s"));\n' % (ws, city))
      f.write('%sif (r < 0) {\n' % (ws))
      fc(sorted, lower, middle - 1, indent + 1)
      
      f.write('%s} else if (r > 0) {\n' % (ws))
      fc(sorted, middle + 1, upper, indent + 1)

      f.write('%s} else {\n' % (ws))
      f.write('%s  return %s;\n' % (ws, defname))
      f.write('%s}\n' % (ws))

  is_first = True
  for area in cities:
    f.write('  %sif (strcmp_P(a, PSTR("%s")) == 0) {\n' % ('' if is_first else 'else ', area))
    cities[area].sort()
    fc(cities[area], 0, len(cities[area]) - 1, 2)
    f.write('  }\n')
    is_first = False
    
  f.write('''  return TZ_Etc_UTC;
}
''')

  f.write('const size_t TZDB::area_maxlength = %d;\n' % (area_maxlength + 1))
  f.write('const size_t TZDB::city_maxlength = %d;\n' % (city_maxlength + 1))