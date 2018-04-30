#pragma once
#include "MetaStrategy.h"

class Manual : public MetaStrategy {

public:
	Manual(void);
	~Manual(void);

	virtual void onStart();
};

