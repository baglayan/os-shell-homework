#!/usr/bin/bash
((cat f1.txt & cat f2.txt) | grep hw) & ((cat f1.txt & cat f2.txt) | grep odev)