from pathlib import Path
import csv, shutil, subprocess, sys
ROOT=Path(__file__).resolve().parents[1]
BUILD=ROOT/'build'; OUT=ROOT/'outputs'; GEN=ROOT/'generated'
BUILD.mkdir(exist_ok=True); OUT.mkdir(exist_ok=True); GEN.mkdir(exist_ok=True)
def run(cmd):
    return subprocess.run(cmd, cwd=ROOT, text=True, capture_output=True, shell=True)
rows=[]
build=run('gcc -std=c11 -Wall -Wextra -O2 src\\main.c -o build\\resultlangc.exe')
(OUT/'build_output.txt').write_text(build.stdout+build.stderr, encoding='utf-8')
if build.returncode: print(build.stdout+build.stderr); sys.exit(build.returncode)
expected={
'01_basic_assignment.rl':['Rahim','87.00'],
'02_precedence.rl':['20.00'],
'03_total_average.rl':['245.00','81.67'],
'04_passing_grade.rl':['Pass'],
'05_failing_grade.rl':['Fail'],
'06_boundary_80.rl':['A+'],
'07_comparisons.rl':['true','true'],
'08_string_bool.rl':['Rafsan','true'],
'09_nested_expression.rl':['150.00'],
'10_full_result_demo.rl':['A+'],
}
tid=1
for src in sorted((ROOT/'examples/valid').glob('*.rl')):
    tdir=OUT/src.stem
    if tdir.exists(): shutil.rmtree(tdir)
    tdir.mkdir()
    comp=run(f'build\\resultlangc.exe "{src}" "{tdir}"')
    gcc=run(f'gcc -std=c11 -Wall -Wextra -Wno-unused-but-set-variable "{tdir}\\generated.c" -o "{tdir}\\program.exe"') if comp.returncode==0 else None
    exe=run(f'"{tdir}\\program.exe"') if gcc and gcc.returncode==0 else None
    actual=(exe.stdout.strip().splitlines() if exe else [])
    ok=comp.returncode==0 and gcc and gcc.returncode==0 and exe and exe.returncode==0 and actual==expected[src.name]
    rows.append([f'T{tid:02d}','valid/pipeline',str(src),'full compile to executable','success + expected output','PASS' if ok else 'FAIL',(comp.stderr+(gcc.stderr if gcc else '')+(exe.stdout if exe else '')).strip(),str(tdir)])
    tid+=1
for src in sorted((ROOT/'examples/invalid').glob('*.rl')):
    tdir=OUT/src.stem
    if tdir.exists(): shutil.rmtree(tdir)
    tdir.mkdir()
    comp=run(f'build\\resultlangc.exe "{src}" "{tdir}"')
    phase='lexical' if 'unknown' in src.name or 'malformed' in src.name else ('syntax' if 'semicolon' in src.name or 'expression' in src.name or 'missing_else' in src.name else 'semantic')
    ok=comp.returncode!=0 and phase in comp.stderr.lower() and not (tdir/'generated.c').exists()
    rows.append([f'T{tid:02d}','invalid/error-handling',str(src),phase+' rejection','compiler rejects without generated executable','PASS' if ok else 'FAIL',comp.stderr.strip(),str(tdir)])
    tid+=1
with (OUT/'test_summary.csv').open('w', newline='', encoding='utf-8') as f:
    w=csv.writer(f); w.writerow(['Test ID','Category','Source filename','Feature','Expected result','Status','Relevant diagnostic or output','Generated files']); w.writerows(rows)
md=['| Test ID | Category | Source | Feature | Expected | Status |','|---|---|---|---|---|---|']
for r in rows: md.append('| '+' | '.join([r[0],r[1],Path(r[2]).name,r[3],r[4],r[5]])+' |')
(OUT/'test_summary.md').write_text('\n'.join(md), encoding='utf-8')
passed=sum(1 for r in rows if r[5]=='PASS')
(OUT/'test_run_transcript.txt').write_text(f'Build return code: {build.returncode}\nTests run: {len(rows)}\nTests passed: {passed}\nTests failed: {len(rows)-passed}\n', encoding='utf-8')
failed = len(rows) - passed
print(f'Tests run: {len(rows)} passed: {passed} failed: {failed}')
if failed == 0:
    print('ALL TESTS PASSED. ResultLang is buildable and working.')
else:
    print('SOME TESTS FAILED. Check outputs/test_summary.md for details.')
sys.exit(0 if failed == 0 else 1)
