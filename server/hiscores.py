"""Merge native Warblade high-score tables without whole-file lost updates.

The browser sends the game's compressed table plus the server revision it loaded.
Only rows added since that revision are considered. SQLite serializes submissions;
the server rebuilds the six top-20 tables after every accepted score.
"""

import hashlib
import sqlite3
import struct
import zlib
from collections import Counter

FILE_SIZE = 0xA1558
TABLE_SIZE = 0x30D4
ENTRY_SIZE = 0x68
TABLE_OFFSETS = (0x8, 0x828, 0x1048, 0x1868, 0x2088, 0x28B0)
SCORE_OFFSET = 0x28
REPLAY_OFFSET = 0x5158
REPLAY_SIZE = 4000 * 32
REPLAY_INDEX = (0, 1, 2, 3, None, 4)
MAX_SNAPSHOTS = 128


def unpack(blob: bytes) -> bytes:
    if len(blob) > 2 * 1024 * 1024:
        raise ValueError("High-score file too large")
    dec = zlib.decompressobj()
    raw = dec.decompress(blob, FILE_SIZE + 1)
    if len(raw) != FILE_SIZE or not dec.eof or dec.unused_data or dec.unconsumed_tail:
        raise ValueError("Invalid high-score file")
    if raw[:4] != b"WARX":
        raise ValueError("Invalid high-score tag")
    return raw


def entries(blob: bytes):
    raw = unpack(blob)
    for table, offset in enumerate(TABLE_OFFSETS):
        for slot in range(20):
            source = raw[offset + slot * ENTRY_SIZE:offset + (slot + 1) * ENTRY_SIZE]
            score = struct.unpack_from("<q", source, SCORE_OFFSET)[0]
            if score <= 0:
                continue
            name = source[:30].split(b"\0", 1)[0].rstrip(b" ")
            if not name or any(c < 32 or c > 126 for c in name) or score > 10**15:
                raise ValueError("Invalid high-score entry")
            # Only values used by the game survive. Discard padding, stale highlights,
            # unknown fields and extreme numbers before another player's game sees them.
            row = bytearray(ENTRY_SIZE)
            row[:32] = name.ljust(31, b"\0") + b"\0"
            struct.pack_into("<q", row, SCORE_OFFSET, score)
            for field in (0x20, 0x30, 0x34, 0x58, 0x5C):
                value = struct.unpack_from("<i", source, field)[0]
                struct.pack_into("<i", row, field, max(0, min(value, 1_000_000_000)))
            for field in (0x50, 0x60):
                value = struct.unpack_from("<q", source, field)[0]
                struct.pack_into("<q", row, field, max(0, min(value, 10**15)))
            date = struct.unpack_from("<8H", source, 0x3C)
            if date[0] <= 9999 and date[1] <= 12 and date[2] <= 6 and date[3] <= 31 and date[4] <= 23 and date[5] <= 59 and date[6] <= 59 and date[7] <= 999:
                struct.pack_into("<8H", row, 0x3C, *date)
            yield table, score, bytes(row)


def blank_board() -> bytes:
    raw = bytearray(FILE_SIZE)
    raw[:4] = b"WARX"
    return zlib.compress(raw)


def render_board(db: sqlite3.Connection) -> bytes:
    raw = bytearray(FILE_SIZE)
    raw[:4] = b"WARX"
    for table, offset in enumerate(TABLE_OFFSETS):
        rows = db.execute("SELECT data,replay FROM score_entries WHERE table_id=? "
                          "ORDER BY score DESC, id ASC LIMIT 20", (table,)).fetchall()
        for slot, row in enumerate(rows):
            raw[offset + slot * ENTRY_SIZE:offset + (slot + 1) * ENTRY_SIZE] = row[0]
        replay_index = REPLAY_INDEX[table]
        if rows and replay_index is not None and rows[0][1]:
            start = REPLAY_OFFSET + replay_index * REPLAY_SIZE
            raw[start:start + REPLAY_SIZE] = rows[0][1]
    return zlib.compress(raw)


def init(db: sqlite3.Connection):
    db.executescript("""
        CREATE TABLE IF NOT EXISTS score_entries (
            id INTEGER PRIMARY KEY, account_id INTEGER NOT NULL REFERENCES accounts(id),
            table_id INTEGER NOT NULL, score INTEGER NOT NULL, digest BLOB NOT NULL UNIQUE,
            data BLOB NOT NULL, replay BLOB
        );
        CREATE INDEX IF NOT EXISTS score_rank ON score_entries(table_id, score DESC, id);
        CREATE TABLE IF NOT EXISTS score_snapshots (
            version INTEGER PRIMARY KEY, data BLOB NOT NULL
        );
    """)
    if not any(row[1] == "replay" for row in db.execute("PRAGMA table_info(score_entries)")):
        db.execute("ALTER TABLE score_entries ADD COLUMN replay BLOB")
    db.execute("INSERT OR IGNORE INTO score_snapshots(version,data) VALUES (0,?)",
               (blank_board(),))


def current(db: sqlite3.Connection):
    return db.execute("SELECT version,data FROM score_snapshots ORDER BY version DESC LIMIT 1").fetchone()


def merge(db: sqlite3.Connection, account_id: int, base_version: int, candidate: bytes):
    base = db.execute("SELECT data FROM score_snapshots WHERE version=?", (base_version,)).fetchone()
    if base is None:
        raise LookupError("High-score revision unavailable; reload the page")
    prior = Counter((table, row) for table, _, row in entries(base[0]))
    incoming_rows = list(entries(candidate))
    incoming = Counter(incoming_rows)
    first_rows = {}
    for table, score, row in incoming_rows:
        first_rows.setdefault(table, row)
    candidate_raw = unpack(candidate)
    inserted = 0
    for (table, score, row), count in incoming.items():
        new_count = count - prior[(table, row)]
        if new_count <= 0:
            continue
        digest = hashlib.sha256(bytes([table]) + row).digest()
        replay_index = REPLAY_INDEX[table]
        replay = None
        if replay_index is not None and first_rows[table] == row:
            start = REPLAY_OFFSET + replay_index * REPLAY_SIZE
            replay = candidate_raw[start:start + REPLAY_SIZE]
        cur = db.execute("INSERT OR IGNORE INTO score_entries(account_id,table_id,score,digest,data,replay) "
                         "VALUES (?,?,?,?,?,?)", (account_id, table, score, digest, row, replay))
        inserted += cur.rowcount
    latest = current(db)
    if not inserted:
        return latest
    for table in range(6):
        db.execute("DELETE FROM score_entries WHERE table_id=? AND id NOT IN "
                   "(SELECT id FROM score_entries WHERE table_id=? "
                   "ORDER BY score DESC, id ASC LIMIT 20)", (table, table))
    version = latest[0] + 1
    data = render_board(db)
    if data == latest[1]:
        return latest
    db.execute("INSERT INTO score_snapshots(version,data) VALUES (?,?)", (version, data))
    db.execute("DELETE FROM score_snapshots WHERE version<?", (version - MAX_SNAPSHOTS + 1,))
    return version, data
