//
// OOP style (OOC by A.T. Schreiner)
//

// file: OOC.h
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

struct Class_ops {
	void *(* ctor)(void *self, va_list *app);
	void  (* dtor)(void *self);
};

struct Class {
	size_t sz;
	struct Class_ops *ops;
};

void *Cnew(const struct Class *class, ...)
{
	void *p = calloc(1, class->sz);       // allocate the sizeof(struct String);

	assert(p);
	*(const struct Class **)p = class;      // Force the conversion of p and set the argument `class` as the value of this pointer.
	if (class->ops->ctor) {
		va_list ap;
		va_start(ap, class);
		p = class->ops->ctor(p, &ap);        // Now what is `p` here, a `struct String` or `struct Class`.
						// and if it is `struct Class` then how it convert to `struct String` in `String_ctor` function
						// given below.
		va_end(ap);
	}
	return p;
}

void Cdelete(const void *_class)
{
	const struct Class *class = _class;
	void *p = calloc(1, class->sz);

	assert(p);
	*(const struct Class **)p = class;
	if (class->ops->dtor) {
		class->ops->dtor(p);
	}
}

// file: mystring.h

//#include "OOC.h"

struct String {
	const struct Class *class;  // must be first
	int ref_cnt;
	char *text;
};
struct String_ops;
void String_hold(struct String *self);
void String_put(struct String *self);


static void *String_ctor(void *_self, va_list *app)
{
	struct String *self = _self;
	const char *text = va_arg(*app, const char *);

	self->text = malloc(strlen(text) + 1);
	assert(self->text);
	strcpy(self->text, text);
	self->ref_cnt = 1;

	printf("String Constructor '%s'\n", text);
	return self;
}

static void String_dtor(void *_self)
{
	struct String *self = _self;

	printf("String Destructor '%s'\n",
	       self->text ? self->text : "N/A");
	if (self->text) {
		free(self->text);
		self->text = NULL;
	}
	free(self);
}

void _String_ops_hold(struct String *self)
{
	self->ref_cnt ++;
}

void _String_ops_put(struct String *self)
{
	if (0 != --self->ref_cnt)
		return;
	self->class->ops->dtor(self);
}

static struct String_ops {
	struct Class_ops;

	void (* hold)(struct String *self);
	void (* put)(struct String *self);
} _String_ops = {
	.ctor = String_ctor,
	.dtor = String_dtor,
	.hold = _String_ops_hold,
	.put = _String_ops_put,
};

// Initialization
static const struct Class _String  = {
	.sz = sizeof(struct String),
	.ops = (struct Class_ops *)&_String_ops,
};

const struct Class *CString = &_String;

void String_hold(struct String *self)
{
	struct String_ops *ops = (struct String_ops *)self->class->ops;
	ops->hold(self);
}

void String_put(struct String *self)
{
	struct String_ops *ops = (struct String_ops *)self->class->ops;
	ops->put(self);
}



// file: main.c

//#include "mystring.h"

int main(void)
{
	struct String *aStr = Cnew(CString, "Tom1");
	String_hold(aStr);
	String_put(aStr);
	String_put(aStr);
	//String_put(aStr);
}

