#!/usr/bin/env python3
"""sqlcheck.py ITUNES_DIR [EXPECTED_ITEMS]: open the three SQLite databases of a library with the stock sqlite3 module,
run integrity_check on each and count the tracks. Prints SQL_OK <n> or SQL_FAIL <reason> (exit 1)."""
import os, sqlite3, sys

d = sys.argv[1]
want = int(sys.argv[2]) if len(sys.argv) > 2 else None
itl = os.path.join(d, "iTunes Library.itlp")
try:
    n = None
    for name in ("Library.itdb", "Locations.itdb", "Dynamic.itdb"):
        c = sqlite3.connect("file:%s?mode=ro" % os.path.join(itl, name), uri=True)
        r = c.execute("pragma integrity_check").fetchall()
        if r != [("ok",)]:
            raise RuntimeError("%s integrity_check: %r" % (name, r))
        if name == "Library.itdb":
            n = c.execute("select count(*) from item").fetchone()[0]
            titles = c.execute("select title from item order by pid").fetchall()
        c.close()
    if want is not None and n != want:
        raise RuntimeError("expected %d items, found %d" % (want, n))
    print("SQL_OK %d" % n)
except Exception as e:  # noqa
    print("SQL_FAIL %s" % e)
    sys.exit(1)
