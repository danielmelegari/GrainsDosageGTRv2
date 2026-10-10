#!/bin/bash
set -euo pipefail
temp_dir=$(mktemp -d)
trap 'rm -rf "$temp_dir"' EXIT
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out "$temp_dir/private.pem" 2>/dev/null
openssl rsa -in "$temp_dir/private.pem" -RSAPublicKey_out -outform DER -out "$temp_dir/public.der" 2>/dev/null
printf 'GRAINS-DOSAGE-V1:0123456789abcdef0123456789abcdef' > "$temp_dir/message"
openssl dgst -sha256 -sign "$temp_dir/private.pem" -out "$temp_dir/signature" "$temp_dir/message"
python3 - "$temp_dir" <<'PY'
import base64,pathlib,sys
p=pathlib.Path(sys.argv[1])
(p/'license_test_key.h').write_text('namespace aztec::license { inline constexpr unsigned char publicKey[]={'+','.join(map(str,(p/'public.der').read_bytes()))+'}; }')
(p/'serial').write_text('GDS1.'+base64.b64encode((p/'message').read_bytes()).decode()+'.'+base64.b64encode((p/'signature').read_bytes()).decode())
PY
clang++ -std=c++17 -fobjc-arc -mmacosx-version-min=10.14 -DGRAINS_COMMERCIAL=1 -DGRAINS_TRIAL=1 -DGRAINS_LICENSE_TEST=1 \
  -Isrc -I"$temp_dir" src/license_mac.mm tools/license_smoke.mm -framework Foundation -framework Security -o "$temp_dir/test"
"$temp_dir/test" "$temp_dir/serial"
