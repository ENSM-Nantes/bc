#include "LoadingQuotes.hpp"

irr::core::stringw getRandomLoadingQuote(irr::u32 seed){

  const irr::u32 quoteCount = 6;
  irr::core::stringw quotes[quoteCount];

  quotes[0] = L"“A smooth sea never made a skilled sailor.”\n- Franklin D. Roosevelt";
  quotes[1] = L"“The sea, once it casts its spell, holds one in its net of wonder forever.”\n- Jacques Cousteau";
  quotes[2] = L"“The sea is the same as it has been since before men ever went on it in boats.”\n- Ernest Hemingway";
  quotes[3] = L"“There is nothing — absolutely nothing — half so much worth doing as simply messing about in boats.”\n- Kenneth Grahame";
  quotes[4] = L"“He who lets the sea lull him into a sense of false security is in very grave danger.”\n- Hammond Innes";
  quotes[5] = L"“The sea finds out everything you did wrong.”\n- Francis Stokes";

  return quotes[seed % quoteCount];
}
