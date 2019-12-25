#!/usr/bin/env python
#
#  Copyright (c) 2012, ASUSTek, Inc. All Rights Reserved.
#  Written by wade chen Wade1_Chen@asus.com
# 

"""
 bq27xx_translate.py  <input_file_names> -o <output_file_name>

reference by  bq27520_battery_upt_i2c.h


typedef enum _cell_type {
        TYPE_COS_LIGHT=0,
        TYPE_LG,
} cell_type;

struct update_op {
        u32 bq_op;
        u32 off; //OP_WAIT not use
        u32 arg;
} ;

struct bq27xx_dffs_data {
        u32 cell_type;
        u32 num_op;
        struct update_op *op; 
};

#define OP_ROM_READ     0
#define OP_ROM_WRITE    1
#define OP_ROM_CMP      2
#define OP_ROM_END      3
#define OP_I2C_START    4
#define OP_I2C_READ     5
#define OP_I2C_WRITE    6
#define OP_I2C_CMP      7
#define OP_WAIT         8

"""

import getopt
import os, sys
import re
import string
import struct
import shlex
import shutil
import time

TYPE_COS_LIGHT = "TYPE_COS_LIGHT"
TYPE_LG = "TYPE_LG"

BQXX_FW_ADDR = '16'
BQXX_I2C_ADDR = 'AA'

DFFS_READ = 'R:'
DFFS_WRITE = 'W:'
DFFS_CMP = 'C:'
DFFS_WAIT = 'X:'
DFFS_START = 'S:'
DFFS_END = 'E:'

OP_ROM_READ = 0
OP_ROM_WRITE = 1
OP_ROM_CMP = 2
OP_ROM_END = 3
OP_I2C_START = 4
OP_I2C_READ = 5
OP_I2C_WRITE = 6
OP_I2C_CMP = 7
OP_WAIT = 8

bq_op_map = { 
        DFFS_READ:{BQXX_FW_ADDR:OP_ROM_READ, BQXX_I2C_ADDR:OP_I2C_READ},
        DFFS_WRITE:{BQXX_FW_ADDR:OP_ROM_WRITE, BQXX_I2C_ADDR:OP_I2C_WRITE},
        DFFS_CMP:{BQXX_FW_ADDR:OP_ROM_CMP, BQXX_I2C_ADDR:OP_I2C_CMP},
        DFFS_START:OP_I2C_START,
        DFFS_END:OP_ROM_END,
        DFFS_WAIT:OP_WAIT 
}

# output to string buffer 
def do_output2buf(entry_buffer):

  string_buffer = """/*
 *            THIS FILE IS AUTO GENERATED. DONT MODIFY IT.
 */


#include "bq27520_battery_upt_i2c.h"

"""

  #output main content
  for entry in entry_buffer:
    cell_type = entry[0]
    num_op = entry[1]
    in_data_buf = entry[2]

    var_name = "OpBuff_" + cell_type
    title = "char " + var_name + "[] = {\n" 

    remainder = len(in_data_buf) % 8
    remainder_start = 0
    content = ""
    for i in range(0, len(in_data_buf), 8):
      if i+8 >= len(in_data_buf):
        break

      line = [ "0x%02x," % ord(in_data_buf[j]) for j in range(i, i+8) ]
      content = content + "    " + ''.join(line) + "\n"
      remainder_start = i+8

    if remainder != 0:
      line = [ "0x%02x," % ord(in_data_buf[j]) for j in range(remainder_start, remainder_start+remainder) ]
      content = content + "    " + ''.join(line) 

    end_title = """
};


"""

    string_buffer += title + content + end_title

  #output other content
  title = """
struct bq27xx_dffs_data bq27xx_fw[] = {
""" 
  
  content = ""
  for entry in entry_buffer:
    cell_type = entry[0]
    num_op = entry[1]
    var_name = "OpBuff_" + cell_type
    content += """  { 
    .cell_type = %s,
    .num_op = %d,
    .op = %s, 
  },
""" % (cell_type, num_op, var_name)

  end_title = """
};
const unsigned int num_of_op_buf = sizeof(bq27xx_fw)/sizeof(struct bq27xx_dffs_data);

"""

  string_buffer += title + content + end_title

  return string_buffer


def do_2translate(line_buffer):
  global TYPE_COS_LIGHT
  global TYPE_LG

  cell_type = TYPE_LG
  data_buf = ''
  num_op = 0
  for line in line_buffer:
    #print "Process %s ... " % line
    if line[0] == '@':
      str_cell_type = line[1:].strip()
      if str_cell_type == 'G_H':
        cell_type = TYPE_LG
      elif str_cell_type == 'G_L':
        cell_type = TYPE_COS_LIGHT
      else:
        raise RuntimeError, "cell type tokens error." 

      continue

    elif line[0] == '#': #comment
      continue

    #parsing operator
    tokens = line.split(' ')
    if len(tokens) == 0: # empty line ??
      continue

    if len(tokens) < 2:
        raise RuntimeError, "line error !! Too few parameter."

    operator = tokens[0].strip()
    if bq_op_map.has_key(operator) is False:
        raise RuntimeError, "operator %s not mapping." % operator

    if operator ==  DFFS_WAIT:
      wait_time_ms = int(tokens[1])
      offset = 0
      bq_mapping_op = bq_op_map[operator]
      #print "%d %d" % (bq_mapping_op, wait_time_ms)
      bq_op = struct.pack('III' , bq_mapping_op, offset, wait_time_ms)
      data_buf += bq_op
      num_op += 1
      continue

    #process other command
    str_slave_addr = tokens[1].strip()

    if operator != DFFS_START and operator != DFFS_END:
      if bq_op_map[operator].has_key(str_slave_addr) is False:
        raise RuntimeError, "Not found mapping address."

      bq_mapping_op = bq_op_map[operator][str_slave_addr]

    else:
      bq_mapping_op = bq_op_map[operator]

    reg_off = int("0x" + tokens[2], 16)
    for one_byte in tokens[3:]:
      #print "%d 0x%02X 0x%02X" % (bq_mapping_op, reg_off, int("0x" + one_byte, 16))
      bq_op = struct.pack('III' , bq_mapping_op, reg_off, int("0x" + one_byte, 16))
      data_buf += bq_op
      num_op += 1
      reg_off += 1

  return (cell_type, num_op, data_buf)

def do_translate(in_file, out_file):
  print in_file
  print out_file
  out_data_buf = ""
  entry_buffer = []
  for input_file in in_file:
    try:
      lines = [line.strip("\n").strip() for line in open(input_file) ]
    except IOError:
      raise RuntimeError, "Reading %s error." % input_file 

    print "Process %s ... \n" % input_file
    #create data buffer
    (cell_type, num_op, out_data_buf) = do_2translate(lines)
    entry_buffer.append([cell_type, num_op, out_data_buf])

  string_buffer = do_output2buf(entry_buffer)
  #print string_buffer

  try:
    with open(out_file, mode="wb") as f:
      f.write(string_buffer)
  except IOError:
    raise RuntimeError, "Writing %s error." % out_file
  finally:
    f.close()

def do_ti_stuff(argv):
  try:
    opts, args = getopt.getopt(argv, "o:") 
  except getopt.GetoptError, err:
    raise RuntimeError, "Argument format incorrect."

  output_file = ""
  for o, a in opts:
    if o == "-o":
      output_file = a
    else:
      raise RuntimeError, "Unknown argument."

  if len(args) < 1:
    raise RuntimeError, "Arguments not enough."

  input_files = args
  for in_file in input_files:
    if os.path.exists(in_file) is False:
      raise RuntimeError, "Input file %s not found." % in_file

  do_translate(input_files, output_file)


if __name__ == '__main__':
  try:
    do_ti_stuff(sys.argv[1:])
  except RuntimeError, eString:
    print "%s\n" % eString 
    sys.exit(1)



