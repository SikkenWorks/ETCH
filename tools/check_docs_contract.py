#!/usr/bin/env python3
"""ET​CH current documentation release-gate; runs without extra pip dependencies."""
from pathlib import Path
import re
import sys
import html.parser

base=Path(__file__).resolve().parents[1]
errors=[]

def require(condition,message):
    if not condition:errors.append(message)

def read(name):
    p=base/name
    if not p.is_file():
        errors.append(f'missing {name}')
        return ''
    return p.read_text(encoding='utf-8')

readme=read('README.md')
manual=read('USER_MANUAL.md')
cheat=read('CHEAT-SHEET.html')
require('V1.88' in readme,'README lacks current release')
require('PJT32' in readme,'README project codec is stale')
require('FM-1_1500' in readme,'README package identity missing')
require(not re.search(r'changelog|release\s*notes?|version\s*history|what\s*(?:has\s*)?changed|what.s\s*new|development\s*history|version.by.version',readme,re.I),'README has forbidden changelog/history text')
require('USER_MANUAL.md' in readme and 'CHEAT-SHEET.html' in readme,'README documentation links missing')
require('GPL-3.0-only' in readme and 'Felucca' in readme,'README credits/license omitted')
for term in ['V1.88','128','DETAIL','SOFT','EDGE','PHASE WARP','HALF SYMM','20 ms','40 ms','65 ms','WAVE AMP','FX BYPASS','PJT32','FM-1_1500','STORAGE LOCKED']:
    require(term in manual,f'user manual missing {term}')
require('V1.84' not in manual,'stale V1.84 text in manual')
require('V1.88' in cheat and 'V1.84' not in cheat,'cheat sheet version stale')
for term in ['DETAIL','SOFT','EDGE','PHASE WARP','HALF SYMM','20 ms','40 ms','65 ms','WAVE AMP','FX BYPASS','HOME','SAVE']:
    require(term in cheat,f'cheat sheet missing {term}')
require(cheat.count('<script>')==1,'cheat sheet must have ONE JS script')
require('selectstart' in cheat and 'contextmenu' in cheat and 'dragstart' in cheat,'selection/right-click JS missing')
require('id="search"' in cheat and 'data-filter="editing"' in cheat,'search/filter controls missing')
require('user-select:' not in cheat,'text selection disabled through CSS instead of JS')
require(len(re.findall(r'<article\s+class="card"',cheat))>=38,'new cards missing')
for fn in ['docs/DOCUMENTATION-STANDARD.md','docs/WEBINSTALLER-UI-STANDARD.md','AGENTS.md']:
    require((base/fn).exists(),f'missing persistent project rule {fn}')
if errors:
    print('FAIL: ETCH documentation release gate')
    for e in errors:print(' -',e)
    sys.exit(1)
print('PASS: ETCH V1.88 documentation contract; 3 current files, no README changelog, current features and clean HTML script')
