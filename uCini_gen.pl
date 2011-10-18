#!/usr/bin/perl
use strict;

if ($ARGV[0] eq '-h') {
print <<EOF;
Usage: uCini_gen.pl [ <FILES> | -h ]
-h  Print this help text and exit

uCini_gen.pl generates C-code that defines the mapping of data to ini files.
It reads the specification from the FILES specified on the command line,
or from standard input and prints the C code to standard output.

Specification format:
;root
[section]; section_name
name= ; address; type
...

- root is the name of your struct tIni root-object representing an ini-file
- section_name internal name of struct tSection object
- address is in C-format, &foo for a scalar, foo for arrays and functions
- type is any of 
  s[124] signed integer, 1, 2 or 4 bytes
  u[124] unsigned integer
  b[0-7] flag on bit position 0 to 7
  str    string zero terminated 
  fun    access function

Note that the specification is in a valid ini format.
You can have multiple roots, those and sections can show up multiple times.
Sections with the same id can be placed in multiple roots,
but internal section_names must be different.
Values with the same names can be placed in multiple sections,
the actual entry will belong to the most recent section.
EOF
exit 0;
}

# Parse spec
my %roots;
my $root;
my $section;
my $line_no = 1;
while (<>) {
    my $parsed;
    s/[\r\n]//g;
    # Root: 1st word after a leading semicolon
    if (/^;(\w+)/) { 
        $root = $1;
        $parsed = 1;
    }
    # Section: word in brackets, semicolon, word
    if ($root && /^\[(\w+)\]; *(\w+)/) {
        $section = $1;
        $roots{$root}->{$section}->{name} = $2;
        $parsed = 1;
    }
    # Entry: word, equals, optional default, semicolon, address, semicolon, type
    if ($section && /^(\w+) *= *\w*; *(.+); *(\w+)/) {
        $roots{$root}->{$section}->{entries}->{$1}->{address} = $2;
        $roots{$root}->{$section}->{entries}->{$1}->{type} = $3;
        $roots{$root}->{$section}->{entries}->{$1}->{line_no} = $line_no;
        $parsed = 1;
    }
    # Print warning for unrecognized lines
    warn "Line $line_no unrecognized: $_\n" if ($_ && !$parsed);
    $line_no++;
}

# Print C-code
foreach $root (sort keys %roots) {
    my %root_obj = %{$roots{$root}};
    # Print entry table of each section
    foreach $section (sort keys %root_obj) {
        my %sect_obj = %{$root_obj{$section}};
        my %sect_entries = %{$sect_obj{entries}}; 
        print "static const struct tEntry $sect_obj{name} [] = {\n";
        # Print line for each entry
        foreach my $entry (sort keys %sect_entries) {
            my %entry_obj = %{$sect_entries{$entry}};
            $entry_obj{type} =~ /^([subtrfn]+)([0-7]?)$/;
            my ($t, $n)      =  ( $1,         $2       );
            my $type_str = $t eq 'u'   ? "eType_INT"
                         : $t eq 's'   ? "eType_INT+eType_SGND"
                         : $t eq 'b'   ? "eType_FLAG"
                         : $t eq 'str' ? "eType_SZ"
                         : $t eq 'fun' ? "eType_FUNC"
                         :               "";
            if ($t =~ /[us]/ && $n =~ /[124]/ || $t eq 'b' && $n =~ /[0-7]/) { 
                $type_str .= "+$n";
            }
            elsif ($n) {
                warn "Line $entry_obj{line_no} invalid type: $t$n\n";
                delete $sect_obj{entries}->{$entry};
                next;
            }
            print "  { \"$entry\", $entry_obj{address}, $type_str },\n";
        }
        print "};\n";
    }
    # Print section table
    print "static const struct tSection ${root}_sections [] = {\n";
    foreach $section (sort keys %root_obj) {
        my %sect_obj = %{$root_obj{$section}};
        my $sect_size = keys %{$sect_obj{entries}};
        print "  { \"$section\", $sect_obj{name}, $sect_size },\n";
    }
    print "};\n";
    # Print root object
    my $root_size = keys %root_obj;
    print "static const struct tIni $root = {\n";
    print "  ${root}_sections, $root_size \n";
    print "};\n";
}

