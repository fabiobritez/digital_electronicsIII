import pikepdf, pypdf, re, os, json
SRC="/srv/agents/digital_electronicsIII/UM10360.pdf"
OUT="/srv/agents/digital_electronicsIII/manual"
# chapter ranges via pypdf outline
rr=pypdf.PdfReader(SRC); N=len(rr.pages); chapters=[]
def walk(it):
    for x in it:
        if isinstance(x,list): walk(x)
        else:
            t=x.title.strip()
            if t.startswith("Chapter "):
                try: pg=rr.get_destination_page_number(x)
                except: pg=None
                if pg is not None: chapters.append([pg,t])
walk(rr.outline); chapters.sort()
def slug(t):
    s=re.sub(r'^Chapter\s+\d+:\s*','',t).replace("LPC176x/5x","").strip()
    s=re.sub(r'\(.*?\)','',s).strip().lower()
    return re.sub(r'[^a-z0-9]+','-',s).strip('-')[:40]
src=pikepdf.open(SRC); man=[]
for i,(pg,title) in enumerate(chapters):
    num=int(re.search(r'Chapter\s+(\d+)',title).group(1))
    end=(chapters[i+1][0]-1) if i+1<len(chapters) else N-1
    out=pikepdf.new()
    for p in range(pg,end+1): out.pages.append(src.pages[p])
    fn=f"ch{num:02d}_{slug(title)}.pdf"
    out.save(os.path.join(OUT,fn))
    sz=os.path.getsize(os.path.join(OUT,fn))//1024
    man.append({"num":num,"title":re.sub(r'^Chapter \d+: ','',title).replace("LPC176x/5x ",""),
                "file":fn,"pdf_start":pg+1,"pdf_end":end+1,"pages":end-pg+1,"kb":sz})
    print(f"{fn:46s} {sz:5d}KB ({end-pg+1}p)")
json.dump(man,open(os.path.join(OUT,"_manifest.json"),"w"),indent=2)
print("TOTAL:", sum(m['kb'] for m in man)//1024,"MB")
