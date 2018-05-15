#pragma once
#include "MetaStrategy.h"

class ManualSelection : public MetaStrategy {

public:
	ManualSelection(void);
	~ManualSelection(void);

	virtual void onSendText(std::string text);
	virtual void onStart();
};

