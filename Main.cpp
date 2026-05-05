# include <Siv3D.hpp>

struct Mole {
	RectF rect;
	double timer = 0.0;
	bool isActive = false;
	bool isBomb = false;
};

void Main() {
	Window::SetTitle(U"Siv3D もぐら叩き Pro");
	Scene::SetBackground(Palette::Darkgreen);

	// テクスチャ登録
	TextureAsset::Register(U"Mole", Emoji{ U"🐹" });
	TextureAsset::Register(U"Bomb", Emoji{ U"💣" });
	TextureAsset::Register(U"Hole", Emoji{ U"🕳️" });

	// --- 音源の修正 ---
	// システム音の代わりに、サイン波などで短い音をその場で作ります
	const Audio hitSE{ GMInstrument::Woodblock, 72, 0.1s, 0.1s }; // 木魚のような高い音
	const Audio bombSE{ GMInstrument::Timpani, 40, 0.5s, 0.5s };  // 低い爆発風の音

	const Font font{ 30, Typeface::Bold };
	const Font titleFont{ 60, Typeface::Heavy };

	// スコア読み込み
	const FilePath saveFilePath = U"score.json";
	Array<int32> highScores = { 0, 0, 0 };
	if (FileSystem::Exists(saveFilePath)) {
		const JSON json = JSON::Load(saveFilePath);
		if (json.isObject() && json[U"scores"].isArray()) {
			for (size_t i = 0; i < 3; ++i) {
				highScores[i] = json[U"scores"][i].get<int32>();
			}
		}
	}

	const double GameLimitTime = 30.0;
	double remainingTime = GameLimitTime;
	int32 score = 0;
	bool isGameActive = true;
	bool isNewRecord = false;

	Array<Mole> moles;
	for (int32 y = 0; y < 3; ++y) {
		for (int32 x = 0; x < 3; ++x) {
			moles << Mole{ RectF{ 150 + x * 150, 150 + y * 150, 100, 100 } };
		}
	}

	Timer spawnTimer{ 0.5s, StartImmediately::Yes };

	while (System::Update()) {
		if (isGameActive) {
			remainingTime -= Scene::DeltaTime();
			if (remainingTime <= 0) {
				remainingTime = 0;
				isGameActive = false;
				highScores << score;
				highScores.rsort();
				highScores.resize(3);
				if (score > 0 && highScores.includes(score)) isNewRecord = true;
				JSON saveJson;
				saveJson[U"scores"] = highScores;
				saveJson.save(saveFilePath);
			}

			if (spawnTimer.reachedZero()) {
				size_t index = Random(moles.size() - 1);
				if (!moles[index].isActive) {
					moles[index].isActive = true;
					moles[index].timer = 0.8;
					moles[index].isBomb = (Random() < 0.2);
				}
				spawnTimer.restart();
			}

			for (auto& mole : moles) {
				if (mole.isActive) {
					mole.timer -= Scene::DeltaTime();
					if (mole.timer <= 0) mole.isActive = false;

					if (mole.rect.leftClicked()) {
						mole.isActive = false;
						if (mole.isBomb) {
							score = Max(0, score - 30);
							bombSE.playOneShot();
						}
						else {
							score += 10;
							hitSE.playOneShot();
						}
					}
				}
			}
		}
		else if (KeyEnter.down()) {
			remainingTime = GameLimitTime;
			score = 0;
			isNewRecord = false;
			isGameActive = true;
			for (auto& mole : moles) mole.isActive = false;
		}

		// 描画
		for (const auto& mole : moles) {
			TextureAsset(U"Hole").resized(100).drawAt(mole.rect.center());
			if (mole.isActive) {
				double offset = Sin(mole.timer * Math::Pi / 0.8) * 35;
				TextureAsset(mole.isBomb ? U"Bomb" : U"Mole").resized(80).drawAt(mole.rect.center().movedBy(0, -offset));
			}
		}

		font(U"Score: {}"_fmt(score)).draw(20, 20);
		font(U"Time: {:.1f}s"_fmt(remainingTime)).draw(Arg::topRight = Vec2{ 780, 20 });

		if (!isGameActive) {
			Scene::Rect().draw(ColorF{ 0, 0, 0, 0.7 });
			titleFont(U"TIME UP!").drawAt(Scene::Center().movedBy(0, -100), Palette::Yellow);
			for (size_t i = 0; i < highScores.size(); ++i) {
				font(U"{}位: {}"_fmt(i + 1, highScores[i]))
					.drawAt(Scene::Center().x, 240 + i * 45, (isNewRecord && highScores[i] == score) ? Palette::Yellow : Palette::White);
			}
			font(U"Press Enter to Retry").drawAt(Scene::Center().x, 450);
		}
	}
}
