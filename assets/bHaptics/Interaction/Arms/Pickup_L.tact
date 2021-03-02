{
	"project": {
		"createdAt": 1588700875148,
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
		"name": "Arm_Pickup_L",
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
											"endTime": 250,
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
													"intensity": 0.6,
													"time": 0,
													"x": 0,
													"y": 0.02
												},
												{
													"intensity": 0.6,
													"time": 250,
													"x": 0,
													"y": 0.56
												}
											],
											"visible": true
										},
										{
											"movingPattern": "CONST_SPEED",
											"playbackType": "FADE_IN",
											"pointList": [
												{
													"intensity": 0.6,
													"time": 0,
													"x": 0.5,
													"y": 0.01
												},
												{
													"intensity": 0.6,
													"time": 250,
													"x": 0.49,
													"y": 0.55
												}
											],
											"visible": true
										},
										{
											"movingPattern": "CONST_SPEED",
											"playbackType": "FADE_IN",
											"pointList": [
												{
													"intensity": 0.6,
													"time": 0,
													"x": 0.99,
													"y": 0.02
												},
												{
													"intensity": 0.6,
													"time": 250,
													"x": 0.98,
													"y": 0.54
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
											"endTime": 250,
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
						"offsetTime": 250,
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
		"updatedAt": 1588700906413
	},
	"durationMillis": 0,
	"intervalMillis": 20,
	"size": 20
}