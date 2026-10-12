#!/bin/bash
set -euo pipefail
temp_dir=$(mktemp -d)
trap 'rm -rf "$temp_dir"' EXIT
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out "$temp_dir/private.pem" 2>/dev/null
openssl rsa -in "$temp_dir/private.pem" -RSAPublicKey_out -outform DER -out "$temp_dir/public.der" 2>/dev/null
python3 - "$temp_dir" <<'PYCODE'
import base64,pathlib,subprocess,sys
p=pathlib.Path(sys.argv[1])
(p/'license_test_key.h').write_text('namespace aztec::license { inline constexpr unsigned char publicKey[]={'+','.join(map(str,(p/'public.der').read_bytes()))+'}; }')
def issue(name,version,message):
 (p/'message').write_text(message)
 subprocess.run(['openssl','dgst','-sha256','-sign',str(p/'private.pem'),'-out',str(p/'signature'),str(p/'message')],check=True)
 (p/name).write_text(version+'.'+base64.b64encode(message.encode()).decode()+'.'+base64.b64encode((p/'signature').read_bytes()).decode())
license_id='0123456789abcdef0123456789abcdef'
issue('serial','GDS2','GRAINS-DOSAGE-V2:'+license_id+':'+64*'a'+':3')
issue('wrong-device','GDS2','GRAINS-DOSAGE-V2:'+license_id+':'+64*'b'+':3')
issue('wrong-limit','GDS2','GRAINS-DOSAGE-V2:'+license_id+':'+64*'a'+':4')
issue('legacy','GDS1','GRAINS-DOSAGE-V1:'+license_id)
PYCODE
for trial in 0 1; do
clang++ -std=c++17 -fobjc-arc -mmacosx-version-min=10.14 -DGRAINS_COMMERCIAL=1 -DGRAINS_TRIAL="$trial" -DGRAINS_LICENSE_TEST=1 \
  -Isrc -I"$temp_dir" src/license_mac.mm tools/license_smoke.mm -framework Foundation -framework Security -framework IOKit -o "$temp_dir/test"
"$temp_dir/test" "$temp_dir/serial" "$temp_dir/wrong-device" "$temp_dir/wrong-limit" "$temp_dir/legacy"

done
# Exercise the production IOKit path as well as the deterministic signature tests.
clang++ -std=c++17 -fobjc-arc -mmacosx-version-min=10.14 -DGRAINS_COMMERCIAL=1 -DGRAINS_TRIAL=0 \
  -Isrc src/license_mac.mm tools/license_device_probe.mm -framework Foundation -framework Security -framework IOKit -o "$temp_dir/probe"
"$temp_dir/probe"
