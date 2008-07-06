#!/bin/sh

# for Anjuta namespace
vala-gen-introspect libanjuta-1.0 libanjuta/
vapigen --library libanjuta libanjuta/libanjuta-1.0.gi

# for IAnjuta namespace
vala-gen-introspect libanjuta-1.0 libanjuta-interfaces/
vapigen --vapidir . --library libanjuta-interfaces libanjuta-interfaces/libanjuta-1.0.gi

cat libanjuta.vapi libanjuta-interfaces.vapi > ../libanjuta-1.0.vapi
