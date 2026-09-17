#include "LoadingQuotes.hpp"

irr::core::stringw getRandomLoadingQuote(irr::u32 seed){

  const irr::u32 quoteCount = 6;
  irr::core::stringw quotes[quoteCount];

  quotes[0] = L"\u201CA smooth sea never made a skilled sailor.\u201D\n- Franklin D. Roosevelt";
  quotes[1] = L"\u201CThe sea, once it casts its spell, holds one in its net of wonder forever.\u201D\n- Jacques Cousteau";
  quotes[2] = L"\u201CThe sea is the same as it has been since before men ever went on it in boats.\u201D\n- Ernest Hemingway";
  quotes[3] = L"\u201CThere is nothing \u2014 absolutely nothing \u2014 half so much worth doing as simply messing about in boats.\u201D\n- Kenneth Grahame";
  quotes[4] = L"\u201CHe who lets the sea lull him into a sense of false security is in very grave danger.\u201D\n- Hammond Innes";
  quotes[5] = L"\u201CThe sea finds out everything you did wrong.\u201D\n- Francis Stokes";

  return quotes[seed % quoteCount];
}
