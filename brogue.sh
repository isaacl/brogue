#!/usr/bin/env bash
(cd `dirname $0`/bin && ./brogue) || (cd `dirname $0` && make && cd bin && ./brogue)

