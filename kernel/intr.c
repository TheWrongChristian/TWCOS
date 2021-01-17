#include "intr.h"

#if INTERFACE

typedef void (*irq_handler)(void * p);
typedef void (*irq_func)(int irq);

#endif

typedef struct irq_state irq_state;
typedef struct irq_handler_data irq_handler_data;

struct irq_handler_data {
	irq_handler handler;
	void * p;
};

struct irq_state {
	interrupt_monitor_t lock[1];
	int count;
	irq_handler_data * handlers;
};

static GCROOT irq_state interrupts[IRQMAX];

static void intr_runall(int irq)
{
	INTERRUPT_MONITOR_AUTOLOCK(interrupts[irq].lock) {
		for(int i=0; i<interrupts[irq].count; i++) {
			interrupts[irq].handlers[i].handler(interrupts[irq].handlers[i].p);
		}
	}
}

interrupt_monitor_t * intr_irq_lock(int irq)
{
	check_int_bounds(irq, 0, countof(interrupts)-1, "Invalid irq value");
	return interrupts[irq].lock;
}

interrupt_monitor_t * interrupt_monitor_irq(int irq)
{
	return intr_irq_lock(irq);
}

void intr_add(int irq, irq_handler handler, void * p)
{
	INTERRUPT_MONITOR_AUTOLOCK(interrupts[irq].lock) {
		if (0 == interrupts[irq].count) {
			irq_func old = add_irq(irq, intr_runall);
			assert(!old || old == intr_runall);
			add_irq(irq, intr_runall);
		}
		irq_handler_data * h = interrupts[irq].handlers;
		h = realloc(h, (1+interrupts[irq].count)*sizeof(*h));
		interrupts[irq].handlers = h;
		h[interrupts[irq].count].handler = handler;
		h[interrupts[irq].count].p = p;
		interrupts[irq].count++;
	}
}
#if 0
void intr_remove(int irq, irq_handler handler)
{
	SPIN_AUTOLOCK(lock) {
		irq_handler_data * h = interrupts[irq].handlers;
		if (0 == h) {
			continue;
		}
		INTERRUPT_MONITOR_AUTOLOCK(interrupts[irq].lock) {
			interrupts[irq].count--;
			for(int i=0; i<interrupts[irq].count; i++) {
				if (handler == h[i].handler) {
					for(int j=i; j<interrupts[irq].count; j++) {
						h[j] = h[j+1];
					}
				}
			}
		}
	}
}
#endif
