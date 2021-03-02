{
	"project": {
		"createdAt": 1588694961787,
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
		"name": "Grenade_L",
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
											"endTime": 296,
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
											"playbackType": "FADE_IN_OUT",
											"pointList": [
												{
													"intensity": 0.5,
													"time": 0,
													"x": 0,
													"y": 1
												},
												{
													"intensity": 0.5,
													"time": 296,
													"x": 0,
													"y": 0
												}
											],
											"visible": true
										},
										{
											"movingPattern": "CONST_SPEED",
											"playbackType": "FADE_IN_OUT",
											"pointList": [
												{
													"intensity": 0.5,
													"time": 0,
													"x": 1,
													"y": 1
												},
												{
													"intensity": 0.5,
													"time": 296,
													"x": 1,
													"y": 0
												}
											],
											"visible": true
										},
										{
											"movingPattern": "CONST_SPEED",
											"playbackType": "FADE_IN_OUT",
											"pointList": [
												{
													"intensity": 0.5,
													"time": 0,
													"x": 0.5,
													"y": 1
												},
												{
													"intensity": 0.5,
													"time": 296,
													"x": 0.5,
													"y": 0
												}
											],
											"visible": true
										},
										{
											"movingPattern": "CONST_SPEED",
											"playbackType": "FADE_IN_OUT",
											"pointList": [
												{
													"intensity": 0.3,
													"time": 0,
													"x": 0.75,
													"y": 1
												},
												{
													"intensity": 0.3,
													"time": 296,
													"x": 0.75,
													"y": 0
												}
											],
											"visible": true
										},
										{
											"movingPattern": "CONST_SPEED",
											"playbackType": "FADE_IN_OUT",
											"pointList": [
												{
													"intensity": 0.3,
													"time": 0,
													"x": 0.24,
													"y": 1
												},
												{
													"intensity": 0.3,
													"time": 296,
													"x": 0.25,
													"y": 0
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
											"endTime": 296,
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
						"offsetTime": 296,
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
		"updatedAt": 1588694984637
	},
	"durationMillis": 0,
	"intervalMillis": 20,
	"size": 20
}