#!/bin/sh

usage() {
	echo genoutfile.sh
	echo "    [-c <config-file>]"
	echo "    [-k <key-file>]"
	echo "    [-p <key-pass-phrase>]"
	echo "    [-o <file-name>]"
	echo
	echo "Generates Certificate Signing Request. It relies on"
	echo "the subjectAltName extension to provide multiple host"
	echo "name for the certificate signing request."
	echo
	echo "Sample config:"
	echo "[ req ]"
	echo "..."
	echo "req_extensions = san_ext"
	echo "..."
	echo
	echo "[ san_ext ]"
	echo "subjectAltName = @alternate_names"
	echo
	echo "[ alternate_names ]"
	echo "DNS.1 = mail.example.com"
	echo "DNS.2 = dns.example.com"
	echo "DNS.3 = docs.example.com"
	echo "..."
	echo "DNS.n = nth.example.com"
	echo
	exit 1
}

config=
keyfile=
passphrase=
outfile=

while [ $# -gt 0 ]
do
	case $1 in
		-c) config=$2; shift ;;
		-k) keyfile=$2; shift ;;
		-o) outfile=$2; shift ;;
		-p) passphrase="-passin pass:$2"; shift;;
		*) usage ;;
	esac
	shift
done

ignore=${config:? "config file not specified"}
ignore=${keyfile:? "key file not specified"}
ignore=${outfile:? "output file not specified"}

openssl req -new -sha256 -config ${config} -key ${keyfile} ${passphrase} -outform pem -out ${outfile}

if [ $? -eq 0 ]
then
	echo
	openssl req -text -in ${outfile} -noout
fi
