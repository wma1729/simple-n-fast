#!/bin/sh

set -eu

usage() {
	echo genkey.sh
	echo "    [-o <key-file>]"
	echo "    [-p <enc-pass-phrase>]"
	echo
	echo Generates RSA key. The key size is 2048 and the public
	echo exponent is 65537. The key is generated in PEM format
	echo and is encrypted with AES-256 encryption.
	echo
	exit 1
}

outfile=
passphrase=

while [ $# -gt 0 ]
do
	case $1 in
		-o) outfile=$2; shift ;;
		-p) passphrase="-pass pass:$2"; shift;;
		*) usage ;;
	esac
	shift
done

ignore=${outfile?= "output file not specified"}

openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:4096 -pkeyopt rsa_keygen_pubexp:65537 -aes256 ${passphrase} -outform pem -out ${outfile}
