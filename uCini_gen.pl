#!/usr/bin/perl
use strict;

if ($ARGV[0] eq '-h') {
print <<EOF;
Usage: uCini_gen.pl [ -o <OUTPUTFILE> ] <FILES> | -h
-o <OUTPUTFILE>  Write to OUTPUTFILE instead of stdout
-h               Print this help text and exit

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
- address is var/buff/func name/address in C,
  '&' prefixes are optional and get stripped from addresses,
  then auto-inserted when needed
- type is any of                                               Auto &
  s[124] signed integer, 1, 2 or 4 bytes                       x
  u[124] unsigned integer                                      x
  b[0-7] flag on bit position 0 to 7                           x
  str    string zero terminated                                -
  fun    access function                                       -
  sym    generate access function for a symbol, e.g. bitfield  -

Note that the specification is in a valid ini format.
You can have multiple roots, those and sections can show up multiple times.
Sections with the same id can be placed in multiple roots,
but internal section_names must be different.
Values with the same names can be placed in multiple sections,
the actual entry will belong to the most recent section.
EOF
exit 0;
}

# Check if outputfile specified
my $outputfile;
if ($ARGV[0] eq '-o') {
    shift;
    $outputfile = shift;
}

# Parse spec from stdin or remaining file list in ARGV
my %roots;
my %symbols;
my $root;
my $section;
my $line_no = 1;
while (<>) {
    s/[\r\n]//g;
    # Root: 1st word after a leading semicolon
    if (/^;(\w+)$/) {
        $root = $1;
        $section = '';
    }
    # Section: word in brackets, semicolon, word
    elsif ($root && /^\[(\w+)\]; *(\w+)/) {
        $section = $1;
        $roots{$root}->{$section}->{name} = $2;
    }
    # Entry: word, equals, optional default, semicolon, address, semicolon, type
    elsif ($section && /^(\w+) *= *\w*; *(.+); *(\w+)/) {
        my ($name, $address, $type) = ($1, $2, $3);
        $address =~ s/^&//; # Strip address operator, will be added later
        $type =~ /^([subtrfnym]+)([0-7]?)$/;
        my ($t, $n) = ($1,       $2     );
        my $type_str = $t eq 'u'   && $n =~ /[124]/ ? "$n+eType_INT"
                     : $t eq 's'   && $n =~ /[124]/ ? "$n+eType_INT+eType_SGND"
                     : $t eq 'b'   && $n =~ /[0-7]/ ? "$n+eType_FLAG"
                     : $type eq 'str'               ? "eType_SZ"
                     : $type eq 'fun'               ? "eType_FUNC"
                     : $type eq 'sym'               ? "eType_FUNC"
                     :                                "";
        $address =~ s/^/&/ if $t =~ /^(u|s|b)$/ && $address !~ /&/;
        if ($type eq "sym") {
            my $func = "__".$root."_".$section."_".$name;
            $symbols{$func} = $address;
            $address = $func;
        }
        if ($type_str) {
            $roots{$root}->{$section}->{entries}->{$name}->{address}  = $address;
            $roots{$root}->{$section}->{entries}->{$name}->{type_str} = $type_str;
        }
        else {
            warn "Line $line_no invalid type: $type\n";
        }
    }
    # Print warning for unrecognized lines that are not empty or comments
    elsif (/^ *[^;].*$/) {
        warn "Line $line_no unrecognized: $_\n";
    }
    $line_no++;
}
# Abort if any input file could not be opened
die if ($!);

# Print C-code to stdout or outputfile
if ($outputfile) {
    open STDOUT, ">$outputfile" or die $!;
}
print <<COMMENT;
/*
 * Code generated by uCini_gen.pl. Edit at your own risk!
 * To be included only once into a single C-file, as it contains
 * static const and function definitions, not just declarations.
 */
COMMENT
# Print symbol access function prototypes
foreach (sort keys %symbols) {
    print "static void $_(char *str, int write);\n";
}
print "\n";
# Print ini-data mapping
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
            print "  { \"$entry\", $entry_obj{address}, $entry_obj{type_str} },\n";
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
# Print symbol access function bodies
foreach (sort keys %symbols) {
print <<EOF;

static void $_(char *str, int write)
{
  if (write) {
    scatd(str, $symbols{$_});
  } else {
    long num;
    if (sscand(str, &num)) $symbols{$_} = num;
  }
}
EOF
}
