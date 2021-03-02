{
	"project": {
		"createdAt": 1588717308064,
		"description": "",
		"id": "",
		"layout": {
			"layouts": {
				"ForearmL": [
					{
						"index": 0,
						"x": 0,
						"y": 0
					},
					{
						"index": 1,
						"x": 0.5,
						"y": 0
					},
					{
						"index": 2,
						"x": 1,
						"y": 0
					},
					{
						"index": 3,
						"x": 0,
						"y": 1
					},
					{
						"index": 4,
						"x": 0.5,
						"y": 1
					},
					{
						"index": 5,
						"x": 1,
						"y": 1
					}
				],
				"ForearmR": [
					{
						"index": 0,
						"x": 0,
						"y": 0
					},
					{
						"index": 1,
						"x": 0.5,
						"y": 0
					},
					{
						"index": 2,
						"x": 1,
						"y": 0
					},
					{
						"index": 3,
						"x": 0,
						"y": 1
					},
					{
						"index": 4,
						"x": 0.5,
						"y": 1
					},
					{
						"index": 5,
						"x": 1,
						"y": 1
					}
				]
			},
			"name": "Tactosy2",
			"type": "Tactosy2"
		},
		"mediaFileDuration": 1,
		"name": "Arm_Ammo_L",
		"tracks": [
			{
				"effects": [
					{
						"modes": {
							"ForearmL": {
								"dotMode": {
									"dotConnected": false,
									"feedback": [
										{
											"endTime": 208,
											"playbackType": "NONE",
											"startTime": 0,
											"pointList": []
										}
									]
								},
								"mode": "PATH_MODE",
								"pathMode": {
									"feedback": [
										{
											"movingPattern": "CONST_SPEED",
											"playbackType": "FADE_IN",
											"pointList": [
												{
													"intensity": 0.5,
													"time": 0,
													"x": 0.22,
													"y": 0.66
												},
												{
													"intensity": 0.5,
													"time": 95,
													"x": 0.98,
													"y": 0.36
												},
												{
													"intensity": 0.5,
													"time": 154,
													"x": 0.48,
													"y": 0.48
												},
												{
													"intensity": 0.5,
													"time": 208,
													"x": 0.2,
													"y": 0.12
												}
											],
											"visible": true
										}
									]
								}
							},
							"ForearmR": {
								"dotMode": {
									"dotConnected": false,
									"feedback": [
										{
											"endTime": 208,
											"playbackType": "NONE",
											"startTime": 0,
											"pointList": []
										}
									]
								},
								"mode": "PATH_MODE",
								"pathMode": {
									"feedback": []
								}
							}
						},
						"name": "Effect 1",
						"offsetTime": 208,
						"startTime": 0
					}
				],
				"enable": true
			},
			{
				"enable": true,
				"effects": []
			}
		],
		"updatedAt": 1588717323608
	},
	"durationMillis": 0,
	"intervalMillis": 20,
	"size": 20
}