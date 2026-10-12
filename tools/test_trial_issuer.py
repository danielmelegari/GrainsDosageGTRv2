import datetime as dt
import tempfile
import unittest
from pathlib import Path
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.exceptions import InvalidSignature
from issue_trial_license import DURATION, issue, keygen, public_blob

class IssuerTests(unittest.TestCase):
    def test_signing_and_tampering(self):
        with tempfile.TemporaryDirectory() as folder:
            key_path, header = Path(folder)/'key.pem', Path(folder)/'public.h'
            keygen(key_path, header)
            with self.assertRaises(ValueError):
                keygen(key_path, header)
            start=dt.datetime(2026,10,12,tzinfo=dt.timezone.utc)
            result=issue(key_path,'a'*64,start)
            lines=result.splitlines(keepends=True)
            self.assertEqual(int(lines[3])-int(lines[2]),DURATION)
            key=serialization.load_pem_private_key(key_path.read_bytes(),None)
            payload=b''.join(lines[:4]); signature=bytes.fromhex(lines[4].decode().strip())
            key.public_key().verify(signature,payload,padding.PKCS1v15(),hashes.SHA256())
            with self.assertRaises(InvalidSignature):
                key.public_key().verify(signature,payload.replace(b'aaaa',b'bbbb'),padding.PKCS1v15(),hashes.SHA256())
            self.assertEqual(len(public_blob(key)),283)
            with self.assertRaises(ValueError): issue(key_path,'invalid',start)
            with self.assertRaises(ValueError): issue(key_path,'a'*64,start.replace(tzinfo=None))

if __name__=='__main__': unittest.main()
