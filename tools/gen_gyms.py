#!/usr/bin/env python3
"""Genera gyms.h (lideres de gimnasio, medallas y equipos de las 9 regiones, y sus ligas)
desde tools/gyms_data.json.

    python3 tools/gen_gyms.py

El JSON: {"regions": [{"region", "gyms": [8 x {leader_en, leader_zh, city_en, city_zh,
type, badge_en, badge_zh, team: [3 numeros de pokedex]}], "elite_four": [4 x {name_en,
name_zh, type, team}], "champion": {name_en, name_zh, type, team}}]} en el orden de las
regiones del juego (Kanto ... Paldea). Los equipos salen del anime (o, si no, del juego).
"""
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
TYPES = ['normal', 'fire', 'water', 'grass', 'electric', 'ice', 'fighting', 'poison', 'ground',
         'flying', 'psychic', 'bug', 'rock', 'ghost', 'dragon', 'dark', 'steel', 'fairy']
ORDER = ['kanto', 'johto', 'hoenn', 'sinnoh', 'unova', 'kalos', 'alola', 'galar', 'paldea']


def lit(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


import opencc
_cc = opencc.OpenCC('s2twp')   # Simplified -> Traditional (Taiwan): pip install opencc-python-reimplemented


def zht(zh):
    return _cc.convert(zh)


def tipo(t):
    t = (t or '').lower()
    return f'TY_{t.upper()}' if t in TYPES else '0xFF'  # 0xFF: varios (campeones)


def persona(en, zh, t, team):
    assert len(team) == 3 and all(1 <= d <= 1025 for d in team), (en, team)
    return f'{{ {lit(en)}, {lit(zh)}, {lit(zht(zh))}, {tipo(t)}, {{ {team[0]}, {team[1]}, {team[2]} }} }}'


def main():
    data = json.load(open(os.path.join(HERE, 'gyms_data.json'), encoding='utf-8'))
    regiones = {r['region']: r for r in data['regions']}
    assert [r['region'] for r in data['regions']] == ORDER, 'regiones en otro orden'
    out = ['#pragma once',
           '// GENERADO por tools/gen_gyms.py desde tools/gyms_data.json - no editar.',
           '// Gimnasios de las 9 regiones (8 cada una) y sus ligas (alto mando + campeon), con',
           '// los nombres oficiales en ingles y chino simplificado, el chino tradicional (convertido con OpenCC s2twp) y equipos del anime.',
           '#include <stdint.h>', '#include "sport.h"', '',
           'struct GymPerson {',
           '  const char *en, *zh, *zht;',
           '  uint8_t type;      // TY_* (0xFF: de varios tipos)',
           '  int16_t team[3];   // el mas fuerte, el ultimo',
           '};',
           'struct GymInfo {',
           '  GymPerson leader;',
           '  const char *cityEn, *cityZh, *cityZht, *badgeEn, *badgeZh, *badgeZht;',
           '};', '',
           'static const GymInfo GYMS[9][8] = {']
    for name in ORDER:
        r = regiones[name]
        assert len(r['gyms']) == 8, name
        out.append(f'  {{  // {name}')
        for g in r['gyms']:
            p = persona(g['leader_en'], g['leader_zh'], g['type'], g['team'])
            out.append(f'    {{ {p}, {lit(g["city_en"])}, {lit(g["city_zh"])}, {lit(zht(g["city_zh"]))}, {lit(g["badge_en"])}, '
                       f'{lit(g["badge_zh"])}, {lit(zht(g["badge_zh"]))} }},')
        out.append('  },')
    out += ['};', '', '// alto mando (4) y campeon de cada region',
            'static const GymPerson LEAGUE[9][5] = {']
    for name in ORDER:
        r = regiones[name]
        assert len(r['elite_four']) == 4, name
        out.append(f'  {{  // {name}')
        for e in r['elite_four'] + [r['champion']]:
            out.append(f'    {persona(e["name_en"], e["name_zh"], e.get("type"), e["team"])},')
        out.append('  },')
    out += ['};', '']
    with open(os.path.join(ROOT, 'gyms.h'), 'w', encoding='utf-8') as fh:
        fh.write('\n'.join(out))
    print('gyms.h:', sum(len(regiones[n]['gyms']) for n in ORDER), 'gimnasios,',
          sum(len(regiones[n]['elite_four']) + 1 for n in ORDER), 'de liga')


if __name__ == '__main__':
    main()
