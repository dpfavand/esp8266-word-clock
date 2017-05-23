#ifndef WORDCLOCK_WORDS_H
  #define WORDCLOCK_WORDS_H

  #include <NeoPixelBus.h>

  //  struct and word definitions

  struct ClockWord {
    int row; // y
    int start; // x first letter
    int end; // x last letter
  };

  struct ClockPhrase {
    int size; // number of words
    ClockWord words[6]; // array with max number of words
    RgbColor startColor; // starting color for animation
    RgbColor endColor; // end color for animation
    time_t expires;
  };

  namespace WORD_PREFIX {
    const ClockWord IT {0, 0, 1};
    const ClockWord IS {0, 3, 4};

    const ClockWord FIVE {0, 6, 9};
    const ClockWord TEN {2, 0, 2};
    const ClockWord QUARTER {1, 0, 6};
    const ClockWord TWENTY {2, 3, 8};
    const ClockWord TWENTYFIVE {2, 3, 8}; // WHOOPS. TwentyFive is actually missing, as Five is before Twenty on the screen. Can change in future version
    const ClockWord HALF {3, 0, 3};

    const ClockWord TO {3, 4, 5};
    const ClockWord PAST {3, 6, 9};

  }

  namespace WORD_HOUR {
    const ClockWord ONE {5, 0, 2};
    const ClockWord TWO {9, 0, 2};
    const ClockWord THREE {5, 3, 7};
    const ClockWord FOUR {4, 6, 9};
    const ClockWord FIVE {9, 4, 7};
    const ClockWord SIX {6, 7, 9};
    const ClockWord SEVEN {4, 0, 4};
    const ClockWord EIGHT {8, 0, 4};
    const ClockWord NINE {8, 6, 9};
    const ClockWord TEN {7, 7, 9};
    const ClockWord ELEVEN {6, 0, 5};
    const ClockWord TWELVE {7, 0, 5};
  }

#endif
