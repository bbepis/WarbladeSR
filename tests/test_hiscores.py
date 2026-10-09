"""Native high-score merge invariants; runs with the Python standard library."""

import sqlite3
import struct
import unittest
import zlib

from server import hiscores


def candidate(base, name, score, *, table=0, replay=b""):
    raw = bytearray(hiscores.unpack(base))
    offset = hiscores.TABLE_OFFSETS[table]
    raw[offset:offset + len(name)] = name.encode("ascii")
    struct.pack_into("<q", raw, offset + hiscores.SCORE_OFFSET, score)
    if replay:
        replay_index = (0, 1, 2, 3, None, 4)[table]
        if replay_index is None:
            raise ValueError("Money table has no replay")
        start = hiscores.REPLAY_OFFSET + replay_index * hiscores.REPLAY_SIZE
        raw[start:start + len(replay)] = replay
    return zlib.compress(raw)


class HighScores(unittest.TestCase):
    def setUp(self):
        self.db = sqlite3.connect(":memory:")
        self.db.execute("CREATE TABLE accounts(id INTEGER PRIMARY KEY)")
        self.db.executemany("INSERT INTO accounts(id) VALUES (?)", [(1,), (2,)])
        hiscores.init(self.db)

    def tearDown(self):
        self.db.close()

    def test_concurrent_top_scores_keep_winners_replay(self):
        version, base = hiscores.current(self.db)
        self.assertEqual(version, 0)
        hiscores.merge(self.db, 1, version, candidate(base, "Alice", 100, replay=b"ALICE RUN"))
        result_version, board = hiscores.merge(
            self.db, 2, version, candidate(base, "Bob", 200, replay=b"BOB RUN"))
        self.assertEqual(result_version, 2)
        self.assertEqual([score for _, score, _ in hiscores.entries(board)], [200, 100])
        raw = hiscores.unpack(board)
        self.assertEqual(raw[hiscores.REPLAY_OFFSET:hiscores.REPLAY_OFFSET + 7], b"BOB RUN")

    def test_history_is_bounded_after_many_qualifying_scores(self):
        for score in range(1, 151):
            version, board = hiscores.current(self.db)
            hiscores.merge(self.db, 1, version, candidate(board, "P%03d" % score, score))
        self.assertLessEqual(self.db.execute("SELECT count(*) FROM score_entries").fetchone()[0], 120)
        self.assertLessEqual(self.db.execute("SELECT count(*) FROM score_snapshots").fetchone()[0], 128)
        scores = [score for _, score, _ in hiscores.entries(hiscores.current(self.db)[1])]
        self.assertEqual(scores, list(range(150, 130, -1)))
        with self.assertRaisesRegex(LookupError, "reload"):
            hiscores.merge(self.db, 2, 0, candidate(hiscores.blank_board(), "Stale", 9000))

    def test_score_below_top_twenty_does_not_advance_revision(self):
        raw = bytearray(hiscores.unpack(hiscores.blank_board()))
        for slot in range(20):
            offset = hiscores.TABLE_OFFSETS[0] + slot * hiscores.ENTRY_SIZE
            raw[offset:offset + 4] = ("P%03d" % slot).encode("ascii")
            struct.pack_into("<q", raw, offset + hiscores.SCORE_OFFSET, 200 - slot)
        first_version, first_board = hiscores.merge(self.db, 1, 0, zlib.compress(raw))
        second_version, second_board = hiscores.merge(
            self.db, 2, first_version, candidate(first_board, "Low", 1))
        self.assertEqual((second_version, second_board), (first_version, first_board))
        self.assertEqual(self.db.execute("SELECT count(*) FROM score_entries").fetchone()[0], 20)


if __name__ == "__main__":
    unittest.main()
