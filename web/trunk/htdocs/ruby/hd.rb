#!/usr/bin/ruby -w

=begin

$Id$

hd.rb - dump memory or file in hex and ascii

Copyright (C) 2006 Stefan Weil

Usage: hd.rb filename offset length - dump a file
       hd.rb offset length          - dump memory
       offset, length are optional.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this file; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.

=end

BYTES_PER_LINE = 16
DEFAULT_OFFSET = 0x00000000
DEFAULT_LENGTH = 0x100

filename = '/dev/mem'

if ARGV.size == 0
	puts("Usage: #{__FILE__} [<filename>] [offset [<length>]]")
	exit
end

filename = ARGV.shift
if ! FileTest.exists?(filename)
	ARGV.unshift(filename)
	filename = '/dev/mem'
end

offset = ARGV.shift
if offset.nil?
	offset = DEFAULT_OFFSET
else
	offset = offset.hex
end

length = ARGV.shift
if length.nil?
	length = DEFAULT_LENGTH
else
	length = length.hex
end

lastaddress = offset + length

File.open(filename, 'rb') { |f|
	f.seek(offset)
	while !f.eof? && offset < lastaddress
		$stdout.write '%08x ' % offset
		text = ''
		for i in 0...BYTES_PER_LINE
			c = f.getc
			$stdout.write(' ') if i == (BYTES_PER_LINE / 2)
			$stdout.write(" #{c.nil? ? '  ' : '%02x' % c}")
			if c.nil?
			elsif c >= 32
				text << c.chr
			else
				text << '.'
			end
		end
		offset += BYTES_PER_LINE
		puts "  |#{text}|"
	end
}

# eof
