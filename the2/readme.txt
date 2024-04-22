We used RB7 as rotation, RB6 as submission input.
Also, we had to use PORTG for movement.
	RG0 -> right
	RG2 -> up
	RG3 -> down
	RG4 -> left

We also encountered a problem where very rarely the next piece was submitted instantaneously. We believe this is because our board's RB7 flickers when our finger shakes. We could not replicate this bug reliably, so we think the 50ms delay before reading PORTB has been a suitable solution to this potential issue.
