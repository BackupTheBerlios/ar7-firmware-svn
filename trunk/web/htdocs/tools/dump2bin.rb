#!/usr/bin/ruby

=begin

Copyright (c) 2006 Stefan Weil

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

This program reads hex dumps produced with the bootloader of
DSL routers like Sinus 154 DSL SE and converts them into binary data.

Usage: dump2bin.rb minicom.cap >flashimage.bin

=end

filename = ARGV[0]

filename = "minicom.cap" if filename.nil?

$stderr.puts("dump2bin #{filename}")

mem = Hash.new

$stderr.puts("dump2bin reading...")

File.open(filename) { |f|
	f.each { |line|
		if line[0..1] == '0x'
			addr, data = line.split(' ', 2)
			data = data.split
			if data.size != 16
				$stderr.puts("Zeile ignoriert: #{line}")
				next
			end
			row = ''
			data.each { |byte|
				row << byte.hex.chr
			}
			if row.size != 16
				$stderr.puts("Zeile ignoriert: #{line}")
				next
			end
			if mem[addr] && (mem[addr] != row)
				$stderr.puts("Zeile doppelt: #{line}")
			end
			mem[addr] = row
		end
	}
}

$stderr.puts("dump2bin sorting...")

mem.sort.each { |addr, row|
	$stdout.write(row)
	#~ puts addr
	#~ File.open(addr, 'wb') { |f|
		#~ f.write(row)
	#~ }
}

$stderr.puts("dump2bin finished!")

# eof
