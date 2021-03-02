{
	"project": {
		"createdAt": 1588605979046,
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
		"name": "Swap_R",
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
											"endTime": 400,
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
											"playbackType": "NONE",
											"visible": true,
											"pointList": []
										}
									]
								}
							},
							"ForearmR": {
								"dotMode": {
									"dotConnected": false,
									"feedback": [
										{
											"endTime": 400,
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
													"intensity": 0.3,
													"time": 0,
													"x": 1,
													"y": 0.45
												},
												{
													"intensity": 0.3,
													"time": 400,
													"x": 0,
													"y": 0
												}
											],
											"visible": true
										},
										{
											"movingPattern": "CONST_SPEED",
											"playbackType": "FADE_IN",
											"pointList": [
												{
													"intensity": 0.3,
													"time": 0,
													"x": 1,
													"y": 0.49
												},
												{
													"intensity": 0.3,
													"time": 400,
													"x": 0,
													"y": 1
												}
											],
											"visible": true
										},
										{
											"movingPattern": "CONST_SPEED",
											"playbackType": "FADE_IN",
											"pointList": [
												{
													"intensity": 0.3,
													"time": 0,
													"x": 1,
													"y": 0.5
												},
												{
													"intensity": 0.3,
													"time": 400,
													"x": 0,
													"y": 0.73
												}
											],
											"visible": true
										},
										{
											"movingPattern": "CONST_SPEED",
											"playbackType": "FADE_IN",
											"pointList": [
												{
													"intensity": 0.3,
													"time": 0,
													"x": 1,
													"y": 0.48
												},
												{
													"intensity": 0.3,
													"time": 400,
													"x": 0,
													"y": 0.25
												}
											],
											"visible": true
										},
										{
											"movingPattern": "CONST_SPEED",
											"playbackType": "FADE_IN",
											"pointList": [
												{
													"intensity": 0.3,
													"time": 0,
													"x": 1,
													"y": 0.48
												},
												{
													"intensity": 0.3,
													"time": 400,
													"x": 0,
													"y": 0.48
												}
											],
											"visible": true
										}
									]
								}
							}
						},
						"name": "Effect 1",
						"offsetTime": 400,
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
		"updatedAt": 1588622331006
	},
	"durationMillis": 0,
	"intervalMillis": 20,
	"size": 20
}