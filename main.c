#include "bitfield.h"
#include "signal_handler.h"
#include "torrentfile.h"
#include "tracker.h"
#include <stdio.h>
extern char *filename;
extern Bitmap *bitmap;
int main() {
  // parse_torrentfile("../data/ubuntu-24.04.1-desktop-amd64.iso.torrent");
  set_signal_handler();
  parse_torrentfile("../data/ubuntu-24.10-desktop-amd64.iso.torrent");
  // create_bitfield();
  connect_tracker();
  return 0;
}
