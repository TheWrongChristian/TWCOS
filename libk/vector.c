#include "table.h"

#if INTERFACE

#endif

#define MLTABLE_ENTRIES_LOG2 6
#define MLTABLE_ENTRIES (1<<MLTABLE_ENTRIES_LOG2)

typedef struct mltable_s {
	int level;

	void * d[MLTABLE_ENTRIES];
} mltable_t;

void * mltable_put(mltable_t * table, int i, void * p)
{
	int index = i>>(MLTABLE_ENTRIES_LOG2*table->level) & (MLTABLE_ENTRIES-1);

	if (table->level) {
		if (0 == table->d[index]) {
			table->d[index] = mltable_new(level-1);
		}

		mltable_put(table->d[index], (1<<(MLTABLE_ENTRIES_LOG2*(table->level-1)-1)), p);
	} else {
		void * old = table->d[index];
		table->d[index] = p;
		return old;
	}
}
